// ClientNetworkHandler core: lifecycle (ctor/dtor/tick), connection teardown, the login
// handshake + async session-auth join, chat, and the shared entity lookup helpers used
// by the per-concern handler translation units. The packet handlers themselves live in
// {Connection-here, World, Entity, Player, Screen}PacketHandlers.cpp; see
// ClientNetworkHandlerInternal.hpp for the rationale behind the split.
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandlerInternal.hpp"
#include "net/minecraft/client/auth/microsoft/SecretProtection.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/network/JavaProtocol.hpp"
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerInteractionManager.hpp"
#include "net/minecraft/client/gui/screen/DisconnectedScreen.hpp"
#include "net/minecraft/client/gui/screen/DownloadingTerrainScreen.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/inventory/SimpleInventory.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include <memory>
#include <utility>
namespace net::minecraft::client::multiplayer {
using namespace detail;
ClientNetworkHandler::ClientNetworkHandler(client::Minecraft* minecraft) : minecraft(minecraft) {}
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
    sendPacket(::net::minecraft::network::java::makeClientLoginHello(minecraft->session.username));
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
  minecraft->setWorld(world);
  if(minecraft->player != nullptr) {
    minecraft->player->dimensionId = dimensionId;
    minecraft->player->id = playerEntityId;
  }
  minecraft->setScreen(std::make_unique<client::gui::screen::DownloadingTerrainScreen>(this));
}
void ClientNetworkHandler::onHello(const LoginHelloPacket& packet) {
  const ::net::minecraft::network::java::ServerLogin login =
      ::net::minecraft::network::java::decodeServerLogin(packet);
  onHello(login.worldSeed, login.dimensionId, login.playerEntityId);
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
  std::string handshakeName = packet.name;
  const std::size_t modsMarker = handshakeName.find(";mods=");
  if(modsMarker != std::string::npos) {
    const std::string requiredCsv = handshakeName.substr(modsMarker + 6);
    handshakeName = handshakeName.substr(0, modsMarker);
    const std::vector<std::string> required = mod::runtime::WorldRequiredMods::splitCsv(requiredCsv);
    const std::vector<std::string> missing = mod::runtime::WorldRequiredMods::missingMods(required);
    if(!missing.empty()) {
      disconnected = true;
      joinServerCanceled_ = true;
      minecraft->setWorld(nullptr);
      retireOwnedWorld();
      world = nullptr;
      minecraft->setScreen(std::make_unique<client::gui::screen::DisconnectedScreen>(
          "disconnect.disconnected", "disconnect.genericReason",
          std::vector<std::string>{mod::runtime::WorldRequiredMods::requirementMessage(missing)}));
      return;
    }
    std::vector<std::string> activeMods;
    for(const mod::runtime::ModPackage& pkg : mod::runtime::host().packageMods()) {
      if(pkg.active) {
        activeMods.push_back(pkg.id);
      }
    }
    sendPacket(ModListPacket(mod::runtime::WorldRequiredMods::joinCsv(activeMods)));
  }
  if(handshakeName == "-") {
    sendPacket(::net::minecraft::network::java::makeClientLoginHello(minecraft->session.username));
    return;
  }
  {
    std::lock_guard lock(joinServerMutex_);
    if(joinServerState_ == JoinServerState::Pending) {
      return;
    }
    joinServerState_ = JoinServerState::Pending;
  }
  // The connector serializes account refresh before the handshake. Redeeming a
  // rotating refresh token again here can invalidate the connector's result.
  const client::util::Session session = minecraft->session;
  const std::string serverId = handshakeName;
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
void ClientNetworkHandler::setEntityPositionAndAnglesAvoidEntities(entity::Entity* entity, double x, double y, double z,
                                                                   float yaw, float pitch, int steps) {
  if(entity == nullptr) {
    return;
  }
  entity->setPositionAndAnglesAvoidEntities(x, y, z, yaw, pitch, steps);
}
} // namespace net::minecraft::client::multiplayer
