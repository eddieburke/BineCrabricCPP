#include "net/minecraft/server/world/ServerWorldEventListener.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/network/packet/WorldPackets.hpp"
#include "net/minecraft/network/packet/EntityPackets.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/server/entity/EntityTracker.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
namespace net::minecraft::server::world {
ServerWorldEventListener::ServerWorldEventListener(MinecraftServer* server, ServerWorld* world)
    : server_(server), world_(world) {
}
void ServerWorldEventListener::notifyEntityAdded(Entity* entity) {
 if(server_ == nullptr || world_ == nullptr || world_->dimension == nullptr || entity == nullptr) {
  return;
 }
 server_->getEntityTracker(world_->dimension->id).onEntityAdded(entity);
}
void ServerWorldEventListener::notifyEntityRemoved(Entity* entity) {
 if(server_ == nullptr || world_ == nullptr || world_->dimension == nullptr || entity == nullptr) {
  return;
 }
 server_->getEntityTracker(world_->dimension->id).onEntityRemoved(entity);
}
void ServerWorldEventListener::onEntityPickup(Entity* entity, PlayerEntity* collector) {
 if(server_ == nullptr || world_ == nullptr || world_->dimension == nullptr || entity == nullptr ||
    collector == nullptr) {
  return;
 }
 ItemPickupAnimationS2CPacket packet;
 packet.entityId = entity->id;
 packet.collectorEntityId = collector->id;
 server_->getEntityTracker(world_->dimension->id).sendToListeners(entity, packet);
}
void ServerWorldEventListener::blockUpdate(int x, int y, int z) {
 if(server_ == nullptr || world_ == nullptr || world_->dimension == nullptr) {
  return;
 }
 server_->playerManager.markDirty(x, y, z, world_->dimension->id);
}
void ServerWorldEventListener::updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity) {
 if(server_ == nullptr) {
  return;
 }
 server_->playerManager.updateBlockEntity(x, y, z, blockEntity);
}
void ServerWorldEventListener::worldEvent(PlayerEntity* player, int event, int x, int y, int z, int data) {
 if(server_ == nullptr || world_ == nullptr || world_->dimension == nullptr) {
  return;
 }
 WorldEventS2CPacket packet;
 packet.eventId = event;
 packet.x = x;
 packet.y = y;
 packet.z = z;
 packet.data = data;
 server_->playerManager.sendToAround(player,
                                     static_cast<double>(x),
                                     static_cast<double>(y),
                                     static_cast<double>(z),
                                     64.0,
                                     world_->dimension->id,
                                     packet);
}
} // namespace net::minecraft::server::world
