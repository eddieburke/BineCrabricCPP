#include "net/minecraft/server/entity/EntityTrackerEntry.hpp"
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <typeinfo>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/SpawnableEntity.hpp"
#include "net/minecraft/entity/TntEntity.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/projectile/FireballEntity.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/EggEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/mod/lua/LuaModEntity.hpp"
#include "net/minecraft/network/packet/EntityPackets.hpp"
#include "net/minecraft/network/packet/LuaModSyncPacket.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::server::entity {
namespace {
using ::net::minecraft::entity::player::ServerPlayerEntity;
constexpr double kVelocityClamp = 3.9;
int clampVelocityUnits(double velocity) {
 if(velocity < -kVelocityClamp) {
  velocity = -kVelocityClamp;
 }
 if(velocity > kVelocityClamp) {
  velocity = kVelocityClamp;
 }
 return static_cast<int>(velocity * 8000.0);
}
EntityVelocityUpdateS2CPacket makeVelocityPacket(int id, double vx, double vy, double vz) {
 EntityVelocityUpdateS2CPacket packet;
 packet.id = id;
 packet.velocityX = clampVelocityUnits(vx);
 packet.velocityY = clampVelocityUnits(vy);
 packet.velocityZ = clampVelocityUnits(vz);
 return packet;
}
} // namespace
EntityTrackerEntry::EntityTrackerEntry(::net::minecraft::entity::Entity* entity,
                                       int trackedDistanceIn,
                                       int trackingFrequencyIn,
                                       bool alwaysUpdateVelocity)
    : currentTrackedEntity(entity),
      trackedDistance(trackedDistanceIn),
      trackingFrequency(trackingFrequencyIn),
      alwaysUpdateVelocity_(alwaysUpdateVelocity) {
 lastX = MathHelper::floor(entity->x * 32.0);
 lastY = MathHelper::floor(entity->y * 32.0);
 lastZ = MathHelper::floor(entity->z * 32.0);
 lastYaw = MathHelper::floor(entity->yaw * 256.0f / 360.0f);
 lastPitch = MathHelper::floor(entity->pitch * 256.0f / 360.0f);
}
void EntityTrackerEntry::notifyNewLocation(const std::vector<ServerPlayerEntity*>& players) {
 newPlayerDataUpdated = false;
 if(!isInitialized_ || currentTrackedEntity->getSquaredDistance(x_, y_, z_) > 16.0) {
  x_ = currentTrackedEntity->x;
  y_ = currentTrackedEntity->y;
  z_ = currentTrackedEntity->z;
  isInitialized_ = true;
  newPlayerDataUpdated = true;
  updateListeners(players);
 }
 ++ticksSinceLastDismount_;
 if(++ticks % trackingFrequency == 0) {
  const int fixedX = MathHelper::floor(currentTrackedEntity->x * 32.0);
  const int fixedY = MathHelper::floor(currentTrackedEntity->y * 32.0);
  const int fixedZ = MathHelper::floor(currentTrackedEntity->z * 32.0);
  const int fixedYaw = MathHelper::floor(currentTrackedEntity->yaw * 256.0f / 360.0f);
  const int fixedPitch = MathHelper::floor(currentTrackedEntity->pitch * 256.0f / 360.0f);
  const int deltaX = fixedX - lastX;
  const int deltaY = fixedY - lastY;
  const int deltaZ = fixedZ - lastZ;
  const bool moved = std::abs(deltaX) >= 8 || std::abs(deltaY) >= 8 || std::abs(deltaZ) >= 8;
  const bool rotated = std::abs(fixedYaw - lastYaw) >= 8 || std::abs(fixedPitch - lastPitch) >= 8;
  if(deltaX < -128 || deltaX >= 128 || deltaY < -128 || deltaY >= 128 || deltaZ < -128 || deltaZ >= 128 ||
     ticksSinceLastDismount_ > 400) {
   ticksSinceLastDismount_ = 0;
   currentTrackedEntity->x = static_cast<double>(fixedX) / 32.0;
   currentTrackedEntity->y = static_cast<double>(fixedY) / 32.0;
   currentTrackedEntity->z = static_cast<double>(fixedZ) / 32.0;
   EntityPositionS2CPacket packet;
   packet.id = currentTrackedEntity->id;
   packet.x = fixedX;
   packet.y = fixedY;
   packet.z = fixedZ;
   packet.yaw = static_cast<std::int8_t>(fixedYaw);
   packet.pitch = static_cast<std::int8_t>(fixedPitch);
   sendToListeners(packet);
  } else if(moved && rotated) {
   EntityRotateAndMoveRelativeS2CPacket packet;
   packet.id = currentTrackedEntity->id;
   packet.deltaX = static_cast<std::int8_t>(deltaX);
   packet.deltaY = static_cast<std::int8_t>(deltaY);
   packet.deltaZ = static_cast<std::int8_t>(deltaZ);
   packet.yaw = static_cast<std::int8_t>(fixedYaw);
   packet.pitch = static_cast<std::int8_t>(fixedPitch);
   sendToListeners(packet);
  } else if(moved) {
   EntityMoveRelativeS2CPacket packet;
   packet.id = currentTrackedEntity->id;
   packet.deltaX = static_cast<std::int8_t>(deltaX);
   packet.deltaY = static_cast<std::int8_t>(deltaY);
   packet.deltaZ = static_cast<std::int8_t>(deltaZ);
   sendToListeners(packet);
  } else if(rotated) {
   EntityRotateS2CPacket packet;
   packet.id = currentTrackedEntity->id;
   packet.yaw = static_cast<std::int8_t>(fixedYaw);
   packet.pitch = static_cast<std::int8_t>(fixedPitch);
   sendToListeners(packet);
  }
  if(alwaysUpdateVelocity_) {
   const double dvx = currentTrackedEntity->velocityX - velocityX;
   const double dvy = currentTrackedEntity->velocityY - velocityY;
   const double dvz = currentTrackedEntity->velocityZ - velocityZ;
   const double deltaSq = dvx * dvx + dvy * dvy + dvz * dvz;
   constexpr double threshold = 0.02;
   if(deltaSq > threshold * threshold ||
      (deltaSq > 0.0 && currentTrackedEntity->velocityX == 0.0 && currentTrackedEntity->velocityY == 0.0 &&
       currentTrackedEntity->velocityZ == 0.0)) {
    velocityX = currentTrackedEntity->velocityX;
    velocityY = currentTrackedEntity->velocityY;
    velocityZ = currentTrackedEntity->velocityZ;
    sendToListeners(makeVelocityPacket(currentTrackedEntity->id, velocityX, velocityY, velocityZ));
   }
  }
  ::net::minecraft::entity::data::DataTracker& dataTracker = currentTrackedEntity->getDataTracker();
  if(dataTracker.isDirty()) {
   EntityTrackerUpdateS2CPacket packet;
   packet.id = currentTrackedEntity->id;
   packet.trackedValues = dataTracker.getDirtyEntries();
   sendToAround(packet);
  }
  if(auto* modEntity = dynamic_cast<::net::minecraft::mod::lua::LuaModEntity*>(currentTrackedEntity)) {
   if(modEntity->takeDirty()) {
    std::unique_ptr<Packet> packet = modEntity->createUpdatePacket();
    if(packet) {
     if(auto* syncPacket = dynamic_cast<LuaModSyncPacket*>(packet.get())) {
      sendToAround(*syncPacket);
     }
    }
   }
  }
  if(moved) {
   lastX = fixedX;
   lastY = fixedY;
   lastZ = fixedZ;
  }
  if(rotated) {
   lastYaw = fixedYaw;
   lastPitch = fixedPitch;
  }
 }
 if(currentTrackedEntity->velocityModified) {
  sendToAround(makeVelocityPacket(currentTrackedEntity->id,
                                  currentTrackedEntity->velocityX,
                                  currentTrackedEntity->velocityY,
                                  currentTrackedEntity->velocityZ));
  currentTrackedEntity->velocityModified = false;
 }
}
void EntityTrackerEntry::notifyEntityRemoved() {
 EntityDestroyS2CPacket packet;
 packet.id = currentTrackedEntity->id;
 sendToListeners(packet);
}
void EntityTrackerEntry::notifyEntityRemoved(ServerPlayerEntity* player) {
 listeners.erase(player);
}
void EntityTrackerEntry::updateListener(ServerPlayerEntity* player) {
 if(player == nullptr || player == currentTrackedEntity || player->networkHandler == nullptr) {
  return;
 }
 const double dx = player->x - static_cast<double>(lastX / 32);
 const double dz = player->z - static_cast<double>(lastZ / 32);
 if(dx >= static_cast<double>(-trackedDistance) && dx <= static_cast<double>(trackedDistance) &&
    dz >= static_cast<double>(-trackedDistance) && dz <= static_cast<double>(trackedDistance)) {
  if(dynamic_cast<net::minecraft::mod::lua::LuaModEntity*>(currentTrackedEntity) != nullptr &&
     (player->networkHandler == nullptr || !player->networkHandler->modProtocolEnabled())) {
   return;
  }
  if(listeners.find(player) == listeners.end()) {
   listeners.insert(player);
   std::unique_ptr<Packet> spawnPacket = createAddEntityPacket();
   player->networkHandler->sendPacket(std::move(spawnPacket));
   if(auto* modEntity = dynamic_cast<net::minecraft::mod::lua::LuaModEntity*>(currentTrackedEntity)) {
    player->networkHandler->sendPacket(modEntity->createUpdatePacket());
    auto& dt = currentTrackedEntity->getDataTracker();
    if(dt.isDirty()) {
     EntityTrackerUpdateS2CPacket dtPkt;
     dtPkt.id = currentTrackedEntity->id;
     dtPkt.trackedValues = dt.getDirtyEntries();
     player->networkHandler->sendPacket(dtPkt);
    }
   }
   if(alwaysUpdateVelocity_) {
    player->networkHandler->sendPacket(makeVelocityPacket(currentTrackedEntity->id,
                                                          currentTrackedEntity->velocityX,
                                                          currentTrackedEntity->velocityY,
                                                          currentTrackedEntity->velocityZ));
   }
   const std::vector<ItemStack> equipment = currentTrackedEntity->getEquipment();
   if(!equipment.empty()) {
    for(std::size_t slot = 0; slot < equipment.size(); ++slot) {
     EntityEquipmentUpdateS2CPacket packet;
     packet.id = currentTrackedEntity->id;
     packet.slot = static_cast<int>(slot);
     packet.itemRawId = equipment[slot].empty() ? -1 : equipment[slot].itemId;
     packet.itemDamage = equipment[slot].empty() ? 0 : equipment[slot].getDamage();
     player->networkHandler->sendPacket(packet);
    }
   }
   if(auto* trackedPlayer =
          dynamic_cast<::net::minecraft::entity::player::PlayerEntity*>(currentTrackedEntity);
      trackedPlayer != nullptr && trackedPlayer->isSleeping()) {
    PlayerSleepUpdateS2CPacket packet;
    packet.id = currentTrackedEntity->id;
    packet.status = 0;
    packet.x = MathHelper::floor(currentTrackedEntity->x);
    packet.y = MathHelper::floor(currentTrackedEntity->y);
    packet.z = MathHelper::floor(currentTrackedEntity->z);
    player->networkHandler->sendPacket(packet);
   }
  }
 } else if(listeners.find(player) != listeners.end()) {
  listeners.erase(player);
  EntityDestroyS2CPacket packet;
  packet.id = currentTrackedEntity->id;
  player->networkHandler->sendPacket(packet);
 }
}
void EntityTrackerEntry::updateListeners(const std::vector<ServerPlayerEntity*>& players) {
 for(ServerPlayerEntity* player : players) {
  updateListener(player);
 }
}
std::unique_ptr<Packet> EntityTrackerEntry::createAddEntityPacket() const {
 namespace mcentity = ::net::minecraft::entity;
 const auto makeEntitySpawn = [this](int entityType, int entityData) {
  auto packet = std::make_unique<EntitySpawnS2CPacket>();
  packet->id = currentTrackedEntity->id;
  packet->entityType = entityType;
  packet->x = MathHelper::floor(currentTrackedEntity->x * 32.0);
  packet->y = MathHelper::floor(currentTrackedEntity->y * 32.0);
  packet->z = MathHelper::floor(currentTrackedEntity->z * 32.0);
  packet->entityData = entityData;
  if(entityData > 0) {
   packet->velocityX = clampVelocityUnits(currentTrackedEntity->velocityX);
   packet->velocityY = clampVelocityUnits(currentTrackedEntity->velocityY);
   packet->velocityZ = clampVelocityUnits(currentTrackedEntity->velocityZ);
  }
  return packet;
 };
 if(auto* itemEntity = dynamic_cast<mcentity::ItemEntity*>(currentTrackedEntity)) {
  auto packet = std::make_unique<ItemEntitySpawnS2CPacket>();
  packet->id = itemEntity->id;
  packet->itemRawId = itemEntity->stack.itemId;
  packet->itemCount = itemEntity->stack.count;
  packet->itemDamage = itemEntity->stack.getDamage();
  packet->x = MathHelper::floor(itemEntity->x * 32.0);
  packet->y = MathHelper::floor(itemEntity->y * 32.0);
  packet->z = MathHelper::floor(itemEntity->z * 32.0);
  packet->velocityX = static_cast<std::int8_t>(itemEntity->velocityX * 128.0);
  packet->velocityY = static_cast<std::int8_t>(itemEntity->velocityY * 128.0);
  packet->velocityZ = static_cast<std::int8_t>(itemEntity->velocityZ * 128.0);
  itemEntity->x = static_cast<double>(packet->x) / 32.0;
  itemEntity->y = static_cast<double>(packet->y) / 32.0;
  itemEntity->z = static_cast<double>(packet->z) / 32.0;
  return packet;
 }
 if(auto* serverPlayer = dynamic_cast<ServerPlayerEntity*>(currentTrackedEntity)) {
  auto packet = std::make_unique<PlayerSpawnS2CPacket>();
  packet->id = serverPlayer->id;
  packet->name = serverPlayer->name;
  packet->x = MathHelper::floor(serverPlayer->x * 32.0);
  packet->y = MathHelper::floor(serverPlayer->y * 32.0);
  packet->z = MathHelper::floor(serverPlayer->z * 32.0);
  packet->yaw = static_cast<std::int8_t>(serverPlayer->yaw * 256.0f / 360.0f);
  packet->pitch = static_cast<std::int8_t>(serverPlayer->pitch * 256.0f / 360.0f);
  const ItemStack* held = serverPlayer->inventory.getSelectedItem();
  packet->itemRawId = held == nullptr ? 0 : held->itemId;
  return packet;
 }
 if(auto* minecart = dynamic_cast<mcentity::vehicle::MinecartEntity*>(currentTrackedEntity)) {
  if(minecart->type == 0) {
   return makeEntitySpawn(10, 0);
  }
  if(minecart->type == 1) {
   return makeEntitySpawn(11, 0);
  }
  if(minecart->type == 2) {
   return makeEntitySpawn(12, 0);
  }
 }
 if(dynamic_cast<mcentity::vehicle::BoatEntity*>(currentTrackedEntity) != nullptr) {
  return makeEntitySpawn(1, 0);
 }
 if(auto* spawnable = dynamic_cast<mcentity::SpawnableEntity*>(currentTrackedEntity)) {
  (void)spawnable;
  auto* living = dynamic_cast<mcentity::LivingEntity*>(currentTrackedEntity);
  auto packet = std::make_unique<LivingEntitySpawnS2CPacket>();
  packet->id = living->id;
  packet->entityType = static_cast<std::int8_t>(mcentity::EntityRegistry::getRawId(*living));
  packet->x = MathHelper::floor(living->x * 32.0);
  packet->y = MathHelper::floor(living->y * 32.0);
  packet->z = MathHelper::floor(living->z * 32.0);
  packet->yaw = static_cast<std::int8_t>(living->yaw * 256.0f / 360.0f);
  packet->pitch = static_cast<std::int8_t>(living->pitch * 256.0f / 360.0f);
  packet->trackedValues = living->getDataTracker().snapshotEntries();
  return packet;
 }
 if(dynamic_cast<mcentity::projectile::FishingBobberEntity*>(currentTrackedEntity) != nullptr) {
  return makeEntitySpawn(90, 0);
 }
 if(auto* arrow = dynamic_cast<mcentity::projectile::ArrowEntity*>(currentTrackedEntity)) {
  const int ownerId = arrow->owner != nullptr ? dynamic_cast<mcentity::LivingEntity*>(arrow->owner)->id
                                              : currentTrackedEntity->id;
  return makeEntitySpawn(60, ownerId);
 }
 if(dynamic_cast<mcentity::projectile::thrown::SnowballEntity*>(currentTrackedEntity) != nullptr) {
  return makeEntitySpawn(61, 0);
 }
 if(auto* fireball = dynamic_cast<mcentity::projectile::FireballEntity*>(currentTrackedEntity)) {
  auto packet = makeEntitySpawn(63, fireball->owner->id);
  packet->velocityX = static_cast<int>(fireball->powerX * 8000.0);
  packet->velocityY = static_cast<int>(fireball->powerY * 8000.0);
  packet->velocityZ = static_cast<int>(fireball->powerZ * 8000.0);
  return packet;
 }
 if(dynamic_cast<mcentity::projectile::thrown::EggEntity*>(currentTrackedEntity) != nullptr) {
  return makeEntitySpawn(62, 0);
 }
 if(dynamic_cast<mcentity::TntEntity*>(currentTrackedEntity) != nullptr) {
  return makeEntitySpawn(50, 0);
 }
 if(auto* fallingBlock = dynamic_cast<mcentity::FallingBlockEntity*>(currentTrackedEntity)) {
  if(Block::SAND != nullptr && fallingBlock->blockId == Block::SAND->id) {
   return makeEntitySpawn(70, 0);
  }
  if(Block::GRAVEL != nullptr && fallingBlock->blockId == Block::GRAVEL->id) {
   return makeEntitySpawn(71, 0);
  }
 }
 if(auto* painting = dynamic_cast<mcentity::decoration::painting::PaintingEntity*>(currentTrackedEntity)) {
  auto packet = std::make_unique<PaintingEntitySpawnS2CPacket>();
  packet->id = painting->id;
  packet->x = painting->attachmentX;
  packet->y = painting->attachmentY;
  packet->z = painting->attachmentZ;
  packet->facing = painting->facing;
  packet->variant = painting->variant.id != nullptr ? painting->variant.id : "Kebab";
  return packet;
 }
 if(auto* modEntity = dynamic_cast<net::minecraft::mod::lua::LuaModEntity*>(currentTrackedEntity)) {
  auto packet = std::make_unique<EntitySpawnS2CPacket>();
  packet->id = modEntity->id;
  packet->entityType = 99;
  packet->x = MathHelper::floor(modEntity->x * 32.0);
  packet->y = MathHelper::floor(modEntity->y * 32.0);
  packet->z = MathHelper::floor(modEntity->z * 32.0);
  packet->entityData = 0;
  return packet;
 }
 throw std::invalid_argument(std::string("Don't know how to add ") + typeid(*currentTrackedEntity).name() + "!");
}
void EntityTrackerEntry::removeListener(ServerPlayerEntity* player) {
 if(listeners.find(player) != listeners.end()) {
  listeners.erase(player);
  if(player->networkHandler != nullptr) {
   EntityDestroyS2CPacket packet;
   packet.id = currentTrackedEntity->id;
   player->networkHandler->sendPacket(packet);
  }
 }
}
} // namespace net::minecraft::server::entity
