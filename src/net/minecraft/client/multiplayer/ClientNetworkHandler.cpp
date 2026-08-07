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
}
 void ClientNetworkHandler::tick() {
  retiredWorlds_.clear();
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
void ClientNetworkHandler::onHello(std::uint64_t worldSeed, int dimensionId, int playerEntityId, bool remoteLuaModsEnabled) {
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
 ownedWorld_->setLuaModGenerationEnabled(remoteLuaModsEnabled);
 minecraft->setWorld(world);
 if(minecraft->player != nullptr) {
  minecraft->player->dimensionId = dimensionId;
  minecraft->player->id = playerEntityId;
 }
 minecraft->setScreen(std::make_unique<client::gui::screen::DownloadingTerrainScreen>(this));
}

void ClientNetworkHandler::disconnect(const std::string& reason) {
 (void)reason;
 disconnected = true;
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
 minecraft->setWorld(nullptr);
 retireOwnedWorld();
 world = nullptr;
 minecraft->setScreen(std::make_unique<client::gui::screen::DisconnectedScreen>("disconnect.lost", reason, objects));
}

void ClientNetworkHandler::onChatMessage(const ChatMessagePacket& packet) {
 if(minecraft == nullptr) {
  return;
 }
 minecraft->inGameHud.addChatMessage(packet.chatMessage);
}
void ClientNetworkHandler::onDisconnect(const DisconnectPacket& packet) {
 disconnected = true;
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
