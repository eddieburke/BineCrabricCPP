// ClientNetworkHandler core: lifecycle (ctor/dtor/tick), connection teardown, the login
// handshake + async session-auth join, chat, and the shared entity lookup helpers used
// by the per-concern handler translation units. The packet handlers themselves live in
// {Connection-here, World, Entity, Player, Screen}PacketHandlers.cpp; see
// ClientNetworkHandlerInternal.hpp for the rationale behind the split.
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include <filesystem>
#include <memory>
#include <utility>
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/auth/microsoft/SecretProtection.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/client/gui/screen/DisconnectedScreen.hpp"
#include "net/minecraft/client/gui/screen/DownloadingTerrainScreen.hpp"
#include "net/minecraft/client/gui/screen/ServerModDownloadScreen.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandlerInternal.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerInteractionManager.hpp"
#include "net/minecraft/client/session/OfflineIdentity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/inventory/SimpleInventory.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/ModPackageIo.hpp"
#include "net/minecraft/mod/lua/LuaModEntity.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/HandshakeMetadata.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/util/http/HttpClient.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::multiplayer {
using namespace detail;
namespace http = ::net::minecraft::util::http;
namespace {
LuaModSyncPacket makeLuaModListPacket(const std::string& csv) {
 LuaModSyncPacket packet;
 packet.kind = LuaModSyncKind::ClientModList;
 packet.payload.assign(csv.begin(), csv.end());
 return packet;
}
} // namespace
ClientNetworkHandler::ClientNetworkHandler(client::Minecraft* minecraft) : minecraft(minecraft) {
}
ClientNetworkHandler::~ClientNetworkHandler() {
 joinServerCanceled_ = true;
 if(joinServerThread_.joinable()) {
  joinServerThread_.join();
 }
}
void ClientNetworkHandler::processPendingJoinServer() {
 if(minecraft == nullptr || disconnected) {
  return;
 }
 JoinServerState state = JoinServerState::None;
 auth::JoinServerResult result;
 {
  std::lock_guard lock(joinServerMutex_);
  state = joinServerState_;
  if(state == JoinServerState::None || state == JoinServerState::Pending) {
   return;
  }
  result = joinServerResult_;
  joinServerState_ = JoinServerState::None;
 }
 if(state == JoinServerState::Succeeded) {
  sendPacket(LoginHelloPacket{::net::minecraft::client::session::resolveJoinUsername(minecraft->session),
                              kProtocolVersionBeta173});
  return;
 }
 if(connection_ == nullptr) {
  return;
 }
 if(!result.error.empty()) {
  connection_->disconnect("disconnect.genericReason", {"Internal client error: " + result.error});
  return;
 }
 connection_->disconnect("disconnect.loginFailedInfo", {result.responseLine});
}
void ClientNetworkHandler::tick() {
 retiredWorlds_.clear();
 processPendingJoinServer();
 if(disconnected || connection_ == nullptr) {
  return;
 }
 connection_->tick();
 connection_->interrupt();
}
void ClientNetworkHandler::onHello(std::uint64_t worldSeed, int dimensionId, int playerEntityId) {
 if(minecraft == nullptr) {
  return;
 }
 minecraft->interactionManager = std::make_unique<MultiplayerInteractionManager>(minecraft, this);
 if(minecraft->stats != nullptr) {
  minecraft->stats->increment(stat::Stats::JOIN_MULTIPLAYER, 1);
 }
 retireOwnedWorld();
 ownedWorld_ = std::make_unique<ClientWorld>(this, worldSeed, dimensionId);
 world = ownedWorld_.get();
 ownedWorld_->setLuaModGenerationEnabled(remoteLuaModsEnabled_);
 minecraft->setWorld(world);
 if(minecraft->player != nullptr) {
  minecraft->player->dimensionId = dimensionId;
  minecraft->player->id = playerEntityId;
 }
 minecraft->setScreen(std::make_unique<client::gui::screen::DownloadingTerrainScreen>(this));
}
void ClientNetworkHandler::onHello(const LoginHelloPacket& packet) {
 // Java's login packet carries no player entity id; the client's id is assigned later
 // via the spawn packet, so it is 0 here (matches the old decodeServerLogin shim).
 onHello(packet.worldSeed, packet.dimensionId, 0);
}
void ClientNetworkHandler::disconnect(const std::string& reason) {
 (void)reason;
 disconnected = true;
 joinServerCanceled_ = true;
 if(minecraft != nullptr) {
  minecraft->setWorld(nullptr);
 }
 retireOwnedWorld();
 world = nullptr;
 openScreenInventory_.reset();
 openScreenFurnace_.reset();
 openScreenDispenser_.reset();
}
void ClientNetworkHandler::onDisconnected(const std::string& reason, const std::vector<std::string>& objects) {
 if(disconnected || minecraft == nullptr) {
  return;
 }
 disconnected = true;
 joinServerCanceled_ = true;
 minecraft->setWorld(nullptr);
 retireOwnedWorld();
 world = nullptr;
 minecraft->setScreen(std::make_unique<client::gui::screen::DisconnectedScreen>("disconnect.lost", reason, objects));
}
void ClientNetworkHandler::onHandshake(const HandshakePacket& packet) {
 if(minecraft == nullptr || disconnected) {
  return;
 }
 if(!minecraft->options.modsEnabled) {
  remoteServerKind_ = RemoteServerKind::JavaCompatible;
  remoteLuaModsEnabled_ = false;
  setModProtocolEnabled(false);
  pendingMissingMods_.clear();
  pendingRequiredMods_.clear();
  pendingDownloadUrls_.clear();
  waitingForModDownloadAcceptance_ = false;
  const network::HandshakeMetadata metadata = network::parseHandshakeMetadata(packet.name);
  beginPendingLogin(metadata.serverId);
  return;
 }
 pendingMissingMods_.clear();
 pendingRequiredMods_.clear();
 pendingDownloadUrls_.clear();
 waitingForModDownloadAcceptance_ = false;
 const network::HandshakeMetadata metadata = network::parseHandshakeMetadata(packet.name);
 remoteServerKind_ = metadata.nativeCppMods ? RemoteServerKind::NativeCppMods : RemoteServerKind::JavaCompatible;
 remoteLuaModsEnabled_ = metadata.nativeCppMods && metadata.luaModsEnabled;
 setModProtocolEnabled(remoteLuaModsEnabled_);
 if(!metadata.requiredMods.empty()) {
  const std::vector<std::string> missing = mod::runtime::WorldRequiredMods::missingMods(metadata.requiredMods);
  if(!missing.empty()) {
   pendingMissingMods_ = missing;
   pendingRequiredMods_ = metadata.requiredMods;
   pendingDownloadUrls_ = metadata.downloadUrls;
   bool canDownloadAll = metadata.nativeCppMods;
   for(const std::string& modId : missing) {
    if(!pendingDownloadUrls_.contains(modId)) {
     canDownloadAll = false;
     break;
    }
   }
   if(canDownloadAll) {
    waitingForModDownloadAcceptance_ = true;
    pendingServerId_ = metadata.serverId;
    minecraft->setScreen(std::make_unique<client::gui::screen::ServerModDownloadScreen>(this, missing));
    return;
   }
   disconnected = true;
   joinServerCanceled_ = true;
   minecraft->setWorld(nullptr);
   retireOwnedWorld();
   world = nullptr;
   minecraft->setScreen(std::make_unique<client::gui::screen::DisconnectedScreen>(
       "disconnect.disconnected",
       "disconnect.genericReason",
       std::vector<std::string>{mod::runtime::WorldRequiredMods::requirementMessage(missing)}));
   return;
  }
 }
 if(remoteServerKind_ == RemoteServerKind::NativeCppMods) {
  sendPacket(makeLuaModListPacket(mod::runtime::WorldRequiredMods::joinCsv(activeClientMods())));
 }
 beginPendingLogin(metadata.serverId);
}
void ClientNetworkHandler::onChatMessage(const ChatMessagePacket& packet) {
 if(minecraft == nullptr) {
  return;
 }
 minecraft->inGameHud.addChatMessage(packet.chatMessage);
}
void ClientNetworkHandler::onDisconnect(const DisconnectPacket& packet) {
 disconnected = true;
 joinServerCanceled_ = true;
 if(minecraft != nullptr) {
  minecraft->setWorld(nullptr);
  retireOwnedWorld();
  world = nullptr;
  minecraft->setScreen(std::make_unique<client::gui::screen::DisconnectedScreen>(
      "disconnect.disconnected", "disconnect.genericReason", std::vector<std::string>{packet.reason}));
 }
}
void ClientNetworkHandler::onKeepAlive(const KeepAlivePacket& packet) {
 sendPacket(packet);
}
std::vector<std::string> ClientNetworkHandler::activeClientMods() const {
 std::vector<std::string> activeMods;
 for(const mod::runtime::ModPackage& pkg : mod::runtime::host().packageMods()) {
  if(pkg.active) {
   activeMods.push_back(pkg.id);
  }
 }
 return activeMods;
}
void ClientNetworkHandler::beginPendingLogin(const std::string& serverId) {
 pendingServerId_ = serverId;
 if(serverId == "-") {
  sendPacket(LoginHelloPacket{::net::minecraft::client::session::resolveJoinUsername(minecraft->session),
                              kProtocolVersionBeta173});
  return;
 }
 {
  std::lock_guard lock(joinServerMutex_);
  if(joinServerState_ == JoinServerState::Pending) {
   return;
  }
  joinServerState_ = JoinServerState::Pending;
 }
 const client::util::Session session = minecraft->session;
 if(joinServerThread_.joinable()) {
  joinServerThread_.join();
 }
 joinServerCanceled_ = false;
 joinServerThread_ = std::thread([this, session = std::move(session), serverId]() mutable {
  auth::JoinServerResult result = auth::verifyJoinServer(session, serverId, &joinServerCanceled_);
  msauth::secret::wipeString(session.mpPass);
  if(joinServerCanceled_.load(std::memory_order_acquire)) {
   return;
  }
  std::lock_guard lock(joinServerMutex_);
  joinServerResult_ = std::move(result);
  joinServerState_ = joinServerResult_.ok ? JoinServerState::Succeeded : JoinServerState::Failed;
 });
}
bool ClientNetworkHandler::downloadPendingMods(std::string& error) {
 if(pendingMissingMods_.empty()) {
  return true;
 }
 const std::filesystem::path runDir = mod::runtime::ModHost::defaultRunDirectory();
 const std::filesystem::path tempDir = runDir / "mod-downloads";
 const std::filesystem::path modsDir = mod::runtime::host().modsDirectory();
 std::filesystem::create_directories(tempDir);
 std::filesystem::create_directories(modsDir);
 for(const std::string& modId : pendingMissingMods_) {
  const auto urlIt = pendingDownloadUrls_.find(modId);
  if(urlIt == pendingDownloadUrls_.end()) {
   error = "No download URL available for required mod " + modId;
   return false;
  }
  http::HttpRequest request;
  request.url = urlIt->second;
  request.maxResponseBytes = static_cast<std::size_t>(mod::lua::kMaxModArchiveBytes);
  const http::HttpResponse response = http::httpRequest(request);
  if(!response.ok() || response.body.empty()) {
   error = "Failed to download required mod " + modId;
   return false;
  }
   const std::filesystem::path tempPath = tempDir / (mod::lua::sanitizeName(modId) + ".zip.tmp");
   if(!mod::lua::writeFileBytes(tempPath, response.body)) {
   error = "Could not write temporary download for " + modId;
   return false;
  }
  std::vector<mod::runtime::ZipEntry> entries;
  if(!mod::runtime::buildZipIndex(response.body, entries)) {
   error = "Downloaded file for " + modId + " is not a valid mod zip";
   return false;
  }
  const mod::runtime::ZipEntry* manifestEntry = mod::runtime::findZipEntry(entries, "mod.json");
  if(manifestEntry == nullptr) {
   error = "Downloaded file for " + modId + " is missing mod.json";
   return false;
  }
  mod::runtime::ModPackage pkg;
  const std::vector<std::uint8_t> manifestBytes = mod::runtime::readZipEntryData(response.body, *manifestEntry);
  if(!mod::runtime::parseManifestJson(std::string(manifestBytes.begin(), manifestBytes.end()),
                                      pkg,
                                      tempPath,
                                      mod::runtime::ModPackageSource::Zip,
                                      "mod.json: ") ||
     pkg.id != modId) {
   error = "Downloaded file does not match required mod " + modId;
   return false;
  }
   const std::filesystem::path finalPath = modsDir / (mod::lua::sanitizeName(modId) + ".zip");
   if(!mod::lua::writeFileBytes(finalPath, response.body)) {
   error = "Could not install downloaded mod " + modId;
   return false;
  }
 }
 mod::runtime::host().rescan();
 mod::runtime::host().loadEnabledPackageMods();
 if(!mod::runtime::WorldRequiredMods::missingMods(pendingRequiredMods_).empty()) {
  error = "Downloaded mods are still missing after install";
  return false;
 }
 pendingMissingMods_.clear();
 return true;
}
void ClientNetworkHandler::continuePendingLogin() {
 waitingForModDownloadAcceptance_ = false;
 if(remoteServerKind_ == RemoteServerKind::NativeCppMods) {
  sendPacket(makeLuaModListPacket(mod::runtime::WorldRequiredMods::joinCsv(activeClientMods())));
 }
 beginPendingLogin(pendingServerId_);
}
void ClientNetworkHandler::cancelPendingModPrompt() {
 waitingForModDownloadAcceptance_ = false;
 joinServerCanceled_ = true;
 if(connection_ != nullptr) {
  connection_->disconnect();
 }
 disconnected = true;
}
Entity* ClientNetworkHandler::getEntity(int id) {
 if(minecraft == nullptr || minecraft->player == nullptr || world == nullptr) {
  return nullptr;
 }
 if(id == minecraft->player->id) {
  return minecraft->player;
 }
 if(ClientWorld* clientWorld = asClientWorld(world)) {
  return clientWorld->getEntity(id);
 }
 return nullptr;
}
void ClientNetworkHandler::setEntityPositionAndAnglesAvoidEntities(
    entity::Entity* entity, double x, double y, double z, float yaw, float pitch, int steps) {
 if(entity == nullptr) {
  return;
 }
 entity->setPositionAndAnglesAvoidEntities(x, y, z, yaw, pitch, steps);
}
void ClientNetworkHandler::onLuaModSync(const LuaModSyncPacket& packet) {
 if(!modProtocolEnabled()) {
  return;
 }
 if(packet.kind == LuaModSyncKind::BlockEntitySnapshot) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr) {
   return;
  }
  try {
   const LuaModSnapshot snapshot = readLuaModSnapshotPacket(packet);
   auto* entity = clientWorld->getBlockEntity(snapshot.x, snapshot.y, snapshot.z);
   auto* modEntity = entity != nullptr && entity->id() == snapshot.registryId
                         ? dynamic_cast<mod::lua::LuaModBlockEntity*>(entity)
                         : nullptr;
   if(modEntity == nullptr && clientWorld->isPosLoaded(snapshot.x, snapshot.y, snapshot.z)) {
    auto replacement = std::make_unique<mod::lua::LuaModBlockEntity>(snapshot.registryId);
    modEntity = replacement.get();
    clientWorld->setBlockEntity(snapshot.x, snapshot.y, snapshot.z, std::move(replacement));
   }
   if(modEntity != nullptr) {
    modEntity->data() = snapshot.data;
   }
  } catch(const std::exception&) {
  }
 } else if(packet.kind == LuaModSyncKind::Entity) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr) {
   return;
  }
  try {
   const LuaModSnapshot snapshot = readLuaModSnapshotPacket(packet);
   auto* entity = clientWorld->getEntity(snapshot.id);
   auto* modEntity = entity != nullptr ? dynamic_cast<mod::lua::LuaModEntity*>(entity) : nullptr;
   if(modEntity != nullptr) {
    modEntity->setRegistryId(snapshot.registryId);
    modEntity->setData(snapshot.data);
    const double rx = static_cast<double>(snapshot.x) / 32.0;
    const double ry = static_cast<double>(snapshot.y) / 32.0;
    const double rz = static_cast<double>(snapshot.z) / 32.0;
    const float ryaw = static_cast<float>(snapshot.yaw) * 360.0f / 256.0f;
    const float rpitch = static_cast<float>(snapshot.pitch) * 360.0f / 256.0f;
    setEntityPositionAndAnglesAvoidEntities(modEntity, rx, ry, rz, ryaw, rpitch, 3);
   }
  } catch(const std::exception&) {
  }
 }
}
} // namespace net::minecraft::client::multiplayer
