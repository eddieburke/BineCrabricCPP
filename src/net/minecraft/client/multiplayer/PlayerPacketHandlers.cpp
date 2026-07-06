// ClientNetworkHandler packet handlers for the local player: server position
// corrections, respawn (deferred out of the world tick), spawn position, health,
// sleep and stat increments. Split out of ClientNetworkHandler.cpp for separation of
// concerns; see ClientNetworkHandlerInternal.hpp.
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandlerInternal.hpp"
#include "net/minecraft/network/JavaProtocol.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/screen/DownloadingTerrainScreen.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include <memory>
namespace net::minecraft::client::multiplayer {
using namespace detail;
std::unique_ptr<Packet> makePlayerMoveResponsePacket(const PlayerMovePacket& packet,
                                                     const entity::player::ClientPlayerEntity& player) {
  const auto move = ::net::minecraft::network::java::makeServerboundPlayerMove(
      player.x, player.boundingBox.minY, player.y, player.z, packet.yaw, packet.pitch, packet.onGround,
      packet.changePosition, packet.changeLook);
  if(packet.changePosition && packet.changeLook) {
    auto response = std::make_unique<PlayerMoveFullPacket>();
    ::net::minecraft::network::java::encodeServerboundPlayerMove(*response, move);
    return response;
  }
  if(packet.changePosition) {
    auto response = std::make_unique<PlayerMovePositionAndOnGroundPacket>();
    ::net::minecraft::network::java::encodeServerboundPlayerMove(*response, move);
    return response;
  }
  if(packet.changeLook) {
    auto response = std::make_unique<PlayerMoveLookAndOnGroundPacket>();
    ::net::minecraft::network::java::encodeServerboundPlayerMove(*response, move);
    return response;
  }
  auto response = std::make_unique<PlayerMovePacket>();
  ::net::minecraft::network::java::encodeServerboundPlayerMove(*response, move);
  return response;
}
void ClientNetworkHandler::onPlayerMove(const PlayerMovePacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr) {
    return;
  }
  const auto move = ::net::minecraft::network::java::decodeClientboundPlayerMove(packet);
  entity::player::ClientPlayerEntity* player = minecraft->player;
  double x = player->x;
  double y = player->y;
  double z = player->z;
  float yaw = player->yaw;
  float pitch = player->pitch;
  if(move.changePosition) {
    x = move.x;
    y = move.stanceY;
    z = move.z;
  }
  if(move.changeLook) {
    yaw = move.yaw;
    pitch = move.pitch;
  }
  player->cameraOffset = 0.0f;
  player->velocityZ = 0.0;
  player->velocityY = 0.0;
  player->velocityX = 0.0;
  player->setPositionAndAngles(x, y, z, yaw, pitch);
  sendPacket(makePlayerMoveResponsePacket(packet, *player));
  if(!started) {
    player->prevX = player->x;
    player->prevY = player->y;
    player->prevZ = player->z;
    started = true;
    minecraft->setScreen(nullptr);
  }
}
void ClientNetworkHandler::onPlayerSleepUpdate(const PlayerSleepUpdateS2CPacket& packet) {
  Entity* entity = getEntity(packet.id);
  if(entity == nullptr || packet.status != 0) {
    return;
  }
  if(auto* player = dynamic_cast<entity::player::PlayerEntity*>(entity)) {
    player->trySleep(packet.x, packet.y, packet.z);
  }
}
void ClientNetworkHandler::onPlayerSpawnPosition(const PlayerSpawnPositionS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr || world == nullptr) {
    return;
  }
  minecraft->player->setSpawnPos(Vec3i{packet.x, packet.y, packet.z});
  world->getProperties().setSpawn(packet.x, packet.y, packet.z);
}
void ClientNetworkHandler::onHealthUpdate(const HealthUpdateS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr) {
    return;
  }
  if(auto* multiplayerPlayer = dynamic_cast<MultiplayerClientPlayerEntity*>(minecraft->player)) {
    multiplayerPlayer->damageTo(packet.health);
  } else {
    minecraft->player->health = packet.health;
  }
}
void ClientNetworkHandler::onPlayerRespawn(const PlayerRespawnPacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr || world == nullptr) {
    return;
  }
  if(packet.dimensionRawId != minecraft->player->dimensionId) {
    started = false;
    const std::uint64_t seed = world->getSeed();
    retireOwnedWorld();
    ownedWorld_ = std::make_unique<ClientWorld>(this, seed, packet.dimensionRawId);
    world = ownedWorld_.get();
    minecraft->setWorld(world);
    minecraft->player->dimensionId = packet.dimensionRawId;
    minecraft->setScreen(std::make_unique<client::gui::screen::DownloadingTerrainScreen>(this));
  }
  // Defer the respawn itself: this handler runs inside ClientWorld::tick() (via
  // networkHandler_->tick()), and Minecraft::respawnPlayer -> prepareWorld() flushes
  // the lighting worker synchronously. Flushing from inside the live world's tick,
  // while the worker may hold chunk pins, deadlocks. Minecraft drains this at the top
  // of the next tick, outside world->tick() -- the same context join already runs in.
  pendingRespawn_ = true;
  pendingRespawnDimension_ = packet.dimensionRawId;
}
void ClientNetworkHandler::applyDeferredRespawn() {
  if(!pendingRespawn_) {
    return;
  }
  pendingRespawn_ = false;
  if(minecraft != nullptr) {
    minecraft->respawnPlayer(true, pendingRespawnDimension_);
  }
}
void ClientNetworkHandler::onIncreaseStat(const IncreaseStatS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr) {
    return;
  }
  if(auto* multiplayerPlayer = dynamic_cast<MultiplayerClientPlayerEntity*>(minecraft->player)) {
    multiplayerPlayer->handleIncreaseStat(packet.statId, packet.amount);
  } else {
    minecraft->player->increaseStat(packet.statId, packet.amount);
  }
}
} // namespace net::minecraft::client::multiplayer
