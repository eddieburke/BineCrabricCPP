// ClientNetworkHandler core: lifecycle (ctor/dtor/tick), connection teardown, the login
// handshake + async session-auth join, chat, and the shared entity lookup helpers used
// by the per-concern handler translation units. The packet handlers themselves live in
// {Connection-here, World, Entity, Player, Screen}PacketHandlers.cpp; see
// ClientNetworkHandlerInternal.hpp for the rationale behind the split.
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include <chrono>
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
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaModEntity.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/runtime/ModPackageIo.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/HandshakeMetadata.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
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
 joinServerWork_->cancelled = true;
 joinServerWork_->completed.request_stop();
 if(pendingModWork_ != nullptr) {
  pendingModWork_->cancelled = true;
  pendingModWork_->events.request_stop();
 }
}
void ClientNetworkHandler::processPendingJoinServer() {
 if(minecraft == nullptr || disconnected) {
  return;
 }
 auth::JoinServerResult result;
 if(!joinServerWork_->completed.tryPop(result)) {
  return;
 }
 if(result.ok) {
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
 if(++keepAliveTicks_ % 20 == 0) {
  sendPacket(KeepAlivePacket{});
 }
 // A remote (Java-MP) server streams chunk data in bursts; cap the per-tick
 // drain so a large join backlog cannot freeze the frame loop.
 const Connection::DrainLimit drainLimit{std::chrono::steady_clock::now() + std::chrono::milliseconds(3), 100};
 connection_->setDrainLimit(drainLimit);
 connection_->tick();
 connection_->clearDrainLimit();
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
 onHello(packet.worldSeed, packet.dimensionId, packet.protocolVersion);
}
void ClientNetworkHandler::disconnect(const std::string& reason) {
 (void)reason;
 disconnected = true;
 joinServerWork_->cancelled = true;
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
 joinServerWork_->cancelled = true;
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
    joinServerWork_->cancelled = true;
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
 joinServerWork_->cancelled = true;
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
 const client::util::Session session = minecraft->session;
 const auto work = joinServerWork_;
 if(work->inFlight.exchange(true, std::memory_order_acq_rel)) {
  return;
 }
 work->cancelled = false;
 net::minecraft::util::concurrent::ThreadCoordinator::instance()
     .pool(net::minecraft::util::concurrent::Domain::Io)
     .submit([work, session = std::move(session), serverId]() mutable {
      auth::JoinServerResult result = auth::verifyJoinServer(session, serverId, &work->cancelled);
      msauth::secret::wipeString(session.mpPass);
      if(!work->cancelled.load(std::memory_order_acquire)) {
       work->completed.push(std::move(result));
      }
      work->inFlight.store(false, std::memory_order_release);
     });
}
void ClientNetworkHandler::downloadPendingMods(const std::shared_ptr<PendingModWork>& work,
                                               std::vector<std::string> missing,
                                               std::unordered_map<std::string, std::string> urls,
                                               std::filesystem::path temporaryDirectory,
                                               std::filesystem::path modsDirectory) {
 std::vector<PendingModFile> files;
 std::error_code filesystemError;
 std::filesystem::create_directories(temporaryDirectory, filesystemError);
 if(filesystemError) {
  work->events.push(PendingModEvent{PendingModEvent::Kind::Failed, "Could not create mod download directory"});
  work->running = false;
  return;
 }
 for(const std::string& modId : missing) {
  if(work->cancelled.load(std::memory_order_acquire)) {
   work->running = false;
   return;
  }
  work->events.tryPush(PendingModEvent{PendingModEvent::Kind::Progress, "Downloading " + modId + "..."});
  const auto url = urls.find(modId);
  if(url == urls.end()) {
   work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                    "No download URL available for required mod " + modId});
   work->running = false;
   return;
  }
  http::HttpRequest request;
  request.url = url->second;
  request.maxResponseBytes = static_cast<std::size_t>(mod::lua::kMaxModArchiveBytes);
  const http::HttpResponse response = http::httpRequest(request);
  if(!response.ok() || response.body.empty()) {
   work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                    "Failed to download required mod " + modId});
   work->running = false;
   return;
  }
  const std::filesystem::path temporary = temporaryDirectory / (mod::lua::sanitizeName(modId) + ".zip.tmp");
  std::vector<mod::runtime::ZipEntry> entries;
  if(!mod::runtime::buildZipIndex(response.body, entries)) {
   work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                    "Downloaded file for " + modId + " is not a valid mod zip"});
   work->running = false;
   return;
  }
  const mod::runtime::ZipEntry* manifestEntry = mod::runtime::findZipEntry(entries, "mod.json");
  if(manifestEntry == nullptr) {
   work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                    "Downloaded file for " + modId + " is missing mod.json"});
   work->running = false;
   return;
  }
  mod::runtime::ModPackage package;
  const std::vector<std::uint8_t> manifest = mod::runtime::readZipEntryData(response.body, *manifestEntry);
  if(!mod::runtime::parseManifestJson(std::string(manifest.begin(), manifest.end()),
                                      package,
                                      temporary,
                                      mod::runtime::ModPackageSource::Zip,
                                      "mod.json: ") ||
     package.id != modId) {
   work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                    "Downloaded file does not match required mod " + modId});
   work->running = false;
   return;
  }
  if(!mod::lua::writeFileBytes(temporary, response.body)) {
   work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                    "Could not write temporary download for " + modId});
   work->running = false;
   return;
  }
  files.push_back(PendingModFile{temporary, modsDirectory / (mod::lua::sanitizeName(modId) + ".zip")});
 }
 work->events.push(PendingModEvent{PendingModEvent::Kind::Complete, "Installing...", std::move(files)});
 work->running = false;
}
bool ClientNetworkHandler::startPendingModDownload() {
 if(pendingMissingMods_.empty()) {
  return false;
 }
 if(pendingModWork_ != nullptr && pendingModWork_->running.load(std::memory_order_acquire)) {
  return false;
 }
 pendingModWork_ = std::make_shared<PendingModWork>();
 pendingModWork_->running = true;
 const auto work = pendingModWork_;
 const std::filesystem::path temporaryDirectory =
     mod::runtime::ModHost::defaultRunDirectory() / "mod-downloads";
 const std::filesystem::path modsDirectory = mod::runtime::host().modsDirectory();
 net::minecraft::util::concurrent::ThreadCoordinator::instance()
     .pool(net::minecraft::util::concurrent::Domain::Io)
     .submit([work,
              missing = pendingMissingMods_,
              urls = pendingDownloadUrls_,
              temporaryDirectory,
              modsDirectory]() mutable {
      downloadPendingMods(work,
                          std::move(missing),
                          std::move(urls),
                          temporaryDirectory,
                          modsDirectory);
     });
 return true;
}
ClientNetworkHandler::PendingModDownloadState ClientNetworkHandler::pollPendingModDownload(
    std::string& status,
    std::string& error) {
 if(pendingModWork_ == nullptr) {
  return PendingModDownloadState::Idle;
 }
 PendingModDownloadState state = pendingModWork_->running.load(std::memory_order_acquire)
                                     ? PendingModDownloadState::Running
                                     : PendingModDownloadState::Idle;
 PendingModEvent event;
 while(pendingModWork_->events.tryPop(event)) {
  status = event.text;
  if(event.kind == PendingModEvent::Kind::Progress) {
   state = PendingModDownloadState::Running;
   continue;
  }
  if(event.kind == PendingModEvent::Kind::Failed) {
   error = std::move(event.text);
   return PendingModDownloadState::Failed;
  }
  std::error_code filesystemError;
  for(const PendingModFile& file : event.files) {
   std::filesystem::create_directories(file.destination.parent_path(), filesystemError);
   if(filesystemError) {
    error = "Could not create mod directory";
    return PendingModDownloadState::Failed;
   }
   std::filesystem::copy_file(
       file.temporary, file.destination, std::filesystem::copy_options::overwrite_existing, filesystemError);
   if(filesystemError) {
    error = "Could not install downloaded mod " + file.destination.stem().string();
    return PendingModDownloadState::Failed;
   }
   std::filesystem::remove(file.temporary, filesystemError);
  }
  mod::runtime::host().rescan();
  if(!mod::runtime::WorldRequiredMods::missingMods(pendingRequiredMods_).empty()) {
   error = "Downloaded mods are still missing after install";
   return PendingModDownloadState::Failed;
  }
  pendingMissingMods_.clear();
  return PendingModDownloadState::Succeeded;
 }
 return state;
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
 joinServerWork_->cancelled = true;
 if(pendingModWork_ != nullptr) {
  pendingModWork_->cancelled = true;
  pendingModWork_->events.request_stop();
 }
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
 if(packet.kind == LuaModSyncKind::Entity) {
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
