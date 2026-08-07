// ClientNetworkHandler packet handlers for the local player: server position
// corrections, respawn (deferred out of the world tick), spawn position, health,
// sleep and stat increments. Split out of ClientNetworkHandler.cpp for separation of
// concerns; see ClientNetworkHandlerInternal.hpp.
#include <memory>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/screen/DownloadingTerrainScreen.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandlerInternal.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft::client::multiplayer {
using namespace detail;
std::unique_ptr<Packet> makePlayerMoveResponsePacket(const PlayerMovePacket& packet,
                                                     const entity::player::ClientPlayerEntity& player) {
 // Java ClientNetworkHandler.onPlayerMove echoes packet.y = boundingBox.minY (feet) and
 // packet.eyeHeight = player.y (eye). Since the entity refactor player.y IS the eye, so the
 // feet value must come off the bounding box. The server's teleport confirmation compares
 // this field against teleportTargetY; a 1.62 mismatch leaves it stuck un-teleported and it
 // never runs playerTick(true), which is the only thing that streams chunk data.
 const double feetY = player.boundingBox.minY;
 const double stance = player.getEyeY();
 if(packet.changePosition && packet.changeLook) {
  auto response = std::make_unique<PlayerMoveFullPacket>();
  response->setMove(player.x, feetY, stance, player.z, packet.yaw, packet.pitch, packet.onGround);
  return response;
 }
 if(packet.changePosition) {
  auto response = std::make_unique<PlayerMovePositionAndOnGroundPacket>();
  response->setMove(player.x, feetY, stance, player.z, packet.yaw, packet.pitch, packet.onGround);
  return response;
 }
 if(packet.changeLook) {
  auto response = std::make_unique<PlayerMoveLookAndOnGroundPacket>();
  response->setMove(player.x, feetY, stance, player.z, packet.yaw, packet.pitch, packet.onGround);
  return response;
 }
 auto response = std::make_unique<PlayerMovePacket>();
 response->setMove(player.x, feetY, stance, player.z, packet.yaw, packet.pitch, packet.onGround);
 return response;
}
void ClientNetworkHandler::onPlayerMove(const PlayerMovePacket& packet) {
 if(minecraft == nullptr || minecraft->player == nullptr) {
  return;
 }
 if(pendingRespawn_) {
  pendingRespawnMove_ = std::make_unique<PlayerMovePacket>(packet);
  return;
 }
 entity::player::ClientPlayerEntity* player = minecraft->player;
 double x = player->x;
 double y = player->y;
 double z = player->z;
 float yaw = player->yaw;
 float pitch = player->pitch;
 if(packet.changePosition) {
  x = packet.x;
  y = packet.feetY;
  z = packet.z;
 }
 if(packet.changeLook) {
  yaw = packet.yaw;
  pitch = packet.pitch;
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
  if(minecraft->worldRenderer != nullptr) {
   minecraft->worldRenderer->sections().resetSectionFrontier();
  }
  if(world != nullptr) {
   const int px = MathHelper::floor(player->x);
   const int pz = MathHelper::floor(player->z);
   world->setBlocksDirty(px - 64, 0, pz - 64, px + 64, Chunk::height - 1, pz + 64);
  }
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
 world->setSpawnPos(Vec3i{packet.x, packet.y, packet.z});
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
 pendingRespawn_ = true;
 pendingRespawnDimension_ = packet.dimensionRawId;
 pendingRespawnMove_.reset();
}
void ClientNetworkHandler::applyDeferredRespawn() {
 if(!pendingRespawn_) {
  return;
 }
 pendingRespawn_ = false;
 if(minecraft != nullptr) {
  minecraft->respawnPlayer(true, pendingRespawnDimension_);
 }
 if(pendingRespawnMove_ != nullptr) {
  std::unique_ptr<PlayerMovePacket> move = std::move(pendingRespawnMove_);
  onPlayerMove(*move);
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
