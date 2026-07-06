// ClientNetworkHandler packet handlers for entity tracking: spawns (mobs, objects,
// players, paintings, items, lightning), movement/teleport, velocity, tracker data,
// equipment, status, vehicles, animations and pickup. Split out of
// ClientNetworkHandler.cpp for separation of concerns; see ClientNetworkHandlerInternal.hpp.
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandlerInternal.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/multiplayer/OtherPlayerEntity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/TntEntity.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/projectile/FireballEntity.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/EggEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include <memory>
namespace net::minecraft::client::multiplayer {
using namespace detail;
void ClientNetworkHandler::onItemEntitySpawn(const ItemEntitySpawnS2CPacket& packet) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr) {
    return;
  }
  const double x = static_cast<double>(packet.x) / 32.0;
  const double y = static_cast<double>(packet.y) / 32.0;
  const double z = static_cast<double>(packet.z) / 32.0;
  auto* itemEntity =
      new entity::ItemEntity(clientWorld, x, y, z, ItemStack(packet.itemRawId, packet.itemCount, packet.itemDamage));
  itemEntity->velocityX = static_cast<double>(packet.velocityX) / 128.0;
  itemEntity->velocityY = static_cast<double>(packet.velocityY) / 128.0;
  itemEntity->velocityZ = static_cast<double>(packet.velocityZ) / 128.0;
  itemEntity->trackedPosX = packet.x;
  itemEntity->trackedPosY = packet.y;
  itemEntity->trackedPosZ = packet.z;
  clientWorld->forceEntity(packet.id, itemEntity);
}
void ClientNetworkHandler::onEntitySpawn(const EntitySpawnS2CPacket& packet) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr) {
    return;
  }
  const double x = static_cast<double>(packet.x) / 32.0;
  const double y = static_cast<double>(packet.y) / 32.0;
  const double z = static_cast<double>(packet.z) / 32.0;
  entity::Entity* entity = nullptr;
  switch(packet.entityType) {
  case 10:
    entity = new entity::vehicle::MinecartEntity(clientWorld, x, y, z, 0);
    break;
  case 11:
    entity = new entity::vehicle::MinecartEntity(clientWorld, x, y, z, 1);
    break;
  case 12:
    entity = new entity::vehicle::MinecartEntity(clientWorld, x, y, z, 2);
    break;
  case 90: {
    auto* bobber = new entity::projectile::FishingBobberEntity(clientWorld);
    bobber->setPosition(x, y, z);
    entity = bobber;
    break;
  }
  case 60:
    entity = new entity::projectile::ArrowEntity(clientWorld, x, y, z);
    break;
  case 61:
    entity = new entity::projectile::thrown::SnowballEntity(clientWorld, x, y, z);
    break;
  case 63:
    entity = new entity::projectile::FireballEntity(
        clientWorld, x, y, z, static_cast<double>(packet.velocityX) / 8000.0,
        static_cast<double>(packet.velocityY) / 8000.0, static_cast<double>(packet.velocityZ) / 8000.0);
    break;
  case 62:
    entity = new entity::projectile::thrown::EggEntity(clientWorld, x, y, z);
    break;
  case 1:
    entity = new entity::vehicle::BoatEntity(clientWorld, x, y, z);
    break;
  case 50:
    entity = new entity::TntEntity(clientWorld, x, y, z);
    break;
  case 70:
    entity = new entity::FallingBlockEntity(clientWorld, x, y, z, Block::SAND != nullptr ? Block::SAND->id : 12);
    break;
  case 71:
    entity =
        new entity::FallingBlockEntity(clientWorld, x, y, z, Block::GRAVEL != nullptr ? Block::GRAVEL->id : 13);
    break;
  default:
    break;
  }
  if(entity == nullptr) {
    return;
  }
  entity->trackedPosX = packet.x;
  entity->trackedPosY = packet.y;
  entity->trackedPosZ = packet.z;
  entity->yaw = 0.0f;
  entity->pitch = 0.0f;
  entity->id = packet.id;
  clientWorld->forceEntity(packet.id, entity);
  if(packet.entityData > 0 && packet.entityType != 63) {
    if(packet.entityType == 60) {
      if(Entity* ownerEntity = getEntity(packet.entityData)) {
        if(auto* arrow = dynamic_cast<entity::projectile::ArrowEntity*>(entity)) {
          if(auto* owner = dynamic_cast<entity::LivingEntity*>(ownerEntity)) {
            arrow->owner = owner;
          }
        }
      }
    }
    entity->setVelocityClient(static_cast<double>(packet.velocityX) / 8000.0,
                              static_cast<double>(packet.velocityY) / 8000.0,
                              static_cast<double>(packet.velocityZ) / 8000.0);
  }
}
void ClientNetworkHandler::onLightningEntitySpawn(const GlobalEntitySpawnS2CPacket& packet) {
  if(world == nullptr || packet.type != 1) {
    return;
  }
  const double x = static_cast<double>(packet.x) / 32.0;
  const double y = static_cast<double>(packet.y) / 32.0;
  const double z = static_cast<double>(packet.z) / 32.0;
  auto* lightningEntity = new entity::LightningEntity(world, x, y, z);
  lightningEntity->trackedPosX = packet.x;
  lightningEntity->trackedPosY = packet.y;
  lightningEntity->trackedPosZ = packet.z;
  lightningEntity->yaw = 0.0f;
  lightningEntity->pitch = 0.0f;
  lightningEntity->id = packet.id;
  world->spawnGlobalEntity(lightningEntity);
}
void ClientNetworkHandler::onPaintingEntitySpawn(const PaintingEntitySpawnS2CPacket& packet) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr) {
    return;
  }
  auto* paintingEntity = new entity::decoration::painting::PaintingEntity(clientWorld, packet.x, packet.y, packet.z,
                                                                          packet.facing, packet.variant);
  clientWorld->forceEntity(packet.id, paintingEntity);
}
void ClientNetworkHandler::onEntityVelocityUpdate(const EntityVelocityUpdateS2CPacket& packet) {
  Entity* entity = getEntity(packet.id);
  if(entity == nullptr) {
    return;
  }
  entity->setVelocityClient(static_cast<double>(packet.velocityX) / 8000.0,
                            static_cast<double>(packet.velocityY) / 8000.0,
                            static_cast<double>(packet.velocityZ) / 8000.0);
}
void ClientNetworkHandler::onEntityTrackerUpdate(const EntityTrackerUpdateS2CPacket& packet) {
  Entity* entity = getEntity(packet.id);
  if(entity == nullptr || packet.trackedValues.empty()) {
    return;
  }
  entity->getDataTracker().writeUpdatedEntries(packet.trackedValues);
}
void ClientNetworkHandler::onPlayerSpawn(const PlayerSpawnS2CPacket& packet) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr || minecraft == nullptr) {
    return;
  }
  const double x = static_cast<double>(packet.x) / 32.0;
  const double y = static_cast<double>(packet.y) / 32.0;
  const double z = static_cast<double>(packet.z) / 32.0;
  const float yaw = decodePacketYaw(packet.yaw);
  const float pitch = decodePacketPitch(packet.pitch);
  auto* otherPlayer = new OtherPlayerEntity(minecraft->world, packet.name);
  otherPlayer->trackedPosX = packet.x;
  otherPlayer->prevX = otherPlayer->lastTickX = x;
  otherPlayer->trackedPosY = packet.y;
  otherPlayer->prevY = otherPlayer->lastTickY = y;
  otherPlayer->trackedPosZ = packet.z;
  otherPlayer->prevZ = otherPlayer->lastTickZ = z;
  if(packet.itemRawId == 0) {
    otherPlayer->inventory.main[static_cast<std::size_t>(otherPlayer->inventory.selectedSlot)] = {};
  } else {
    otherPlayer->inventory.main[static_cast<std::size_t>(otherPlayer->inventory.selectedSlot)] =
        ItemStack(packet.itemRawId, 1, 0);
  }
  otherPlayer->setPositionAndAngles(x, y, z, yaw, pitch);
  clientWorld->forceEntity(packet.id, otherPlayer);
}
void ClientNetworkHandler::onEntityPosition(const EntityPositionS2CPacket& packet) {
  Entity* entity = getEntity(packet.id);
  if(entity == nullptr) {
    return;
  }
  entity->trackedPosX = packet.x;
  entity->trackedPosY = packet.y;
  entity->trackedPosZ = packet.z;
  const double x = static_cast<double>(entity->trackedPosX) / 32.0;
  const double y = static_cast<double>(entity->trackedPosY) / 32.0 + 0.015625;
  const double z = static_cast<double>(entity->trackedPosZ) / 32.0;
  const float yaw = decodePacketYaw(packet.yaw);
  const float pitch = decodePacketPitch(packet.pitch);
  setEntityPositionAndAnglesAvoidEntities(entity, x, y, z, yaw, pitch, 3);
}
void ClientNetworkHandler::onEntity(const EntityS2CPacket& packet) {
  Entity* entity = getEntity(packet.id);
  if(entity == nullptr) {
    return;
  }
  entity->trackedPosX += packet.deltaX;
  entity->trackedPosY += packet.deltaY;
  entity->trackedPosZ += packet.deltaZ;
  const double x = static_cast<double>(entity->trackedPosX) / 32.0;
  const double y = static_cast<double>(entity->trackedPosY) / 32.0;
  const double z = static_cast<double>(entity->trackedPosZ) / 32.0;
  const float yaw = packet.rotate ? decodePacketYaw(packet.yaw) : entity->yaw;
  const float pitch = packet.rotate ? decodePacketPitch(packet.pitch) : entity->pitch;
  setEntityPositionAndAnglesAvoidEntities(entity, x, y, z, yaw, pitch, 3);
}
void ClientNetworkHandler::onEntityDestroy(const EntityDestroyS2CPacket& packet) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr) {
    return;
  }
  clientWorld->removeEntity(packet.id);
}
void ClientNetworkHandler::onItemPickupAnimation(const ItemPickupAnimationS2CPacket& packet) {
  if(minecraft == nullptr || world == nullptr) {
    return;
  }
  Entity* entity = getEntity(packet.entityId);
  if(entity == nullptr) {
    return;
  }
  entity::player::PlayerEntity* collector =
      dynamic_cast<entity::player::PlayerEntity*>(getEntity(packet.collectorEntityId));
  if(collector == nullptr) {
    collector = minecraft->player;
  }
  if(collector == nullptr) {
    return;
  }
  world->playSound(entity, "random.pop", 0.2f, ((random.nextFloat() - random.nextFloat()) * 0.7f + 1.0f) * 2.0f);
  world->notifyEntityPickup(entity, collector);
  if(ClientWorld* clientWorld = asClientWorld(world)) {
    clientWorld->removeEntity(packet.entityId);
  }
}
void ClientNetworkHandler::onEntityAnimation(const EntityAnimationPacket& packet) {
  Entity* entity = getEntity(packet.id);
  if(entity == nullptr) {
    return;
  }
  auto* player = dynamic_cast<entity::player::PlayerEntity*>(entity);
  if(packet.animationId == 1) {
    if(player != nullptr) {
      player->swingHand();
    }
  } else if(packet.animationId == 2) {
    entity->animateHurt();
  } else if(packet.animationId == 3) {
    if(player != nullptr) {
      player->wakeUp(false, false, false);
    }
  } else if(packet.animationId == 4) {
    if(auto* otherPlayer = dynamic_cast<OtherPlayerEntity*>(entity)) {
      otherPlayer->spawn();
    }
  }
}
void ClientNetworkHandler::onLivingEntitySpawn(const LivingEntitySpawnS2CPacket& packet) {
  ClientWorld* clientWorld = asClientWorld(world);
  if(clientWorld == nullptr || minecraft == nullptr || minecraft->world == nullptr) {
    return;
  }
  const double x = static_cast<double>(packet.x) / 32.0;
  const double y = static_cast<double>(packet.y) / 32.0;
  const double z = static_cast<double>(packet.z) / 32.0;
  const float yaw = decodePacketYaw(packet.yaw);
  const float pitch = decodePacketPitch(packet.pitch);
  std::unique_ptr<entity::Entity> created =
      entity::EntityRegistry::create(static_cast<int>(packet.entityType), minecraft->world);
  auto* livingEntity = dynamic_cast<entity::LivingEntity*>(created.get());
  if(livingEntity == nullptr) {
    return;
  }
  livingEntity->trackedPosX = packet.x;
  livingEntity->trackedPosY = packet.y;
  livingEntity->trackedPosZ = packet.z;
  livingEntity->id = packet.id;
  livingEntity->setPositionAndAngles(x, y, z, yaw, pitch);
  livingEntity->interpolateOnly = true;
  clientWorld->forceEntity(packet.id, created.release());
  if(!packet.trackedValues.empty()) {
    livingEntity->getDataTracker().writeUpdatedEntries(packet.trackedValues);
  }
}
void ClientNetworkHandler::onEntityVehicleSet(const EntityVehicleSetS2CPacket& packet) {
  Entity* entity = getEntity(packet.id);
  Entity* vehicle = getEntity(packet.vehicleId);
  if(minecraft != nullptr && packet.id == minecraft->player->id) {
    entity = minecraft->player;
  }
  if(entity == nullptr) {
    return;
  }
  // Reject cycle: vehicle must not already be a passenger of entity.
  if(vehicle != nullptr && vehicle->vehicle == entity) {
    return;
  }
  entity->setVehicle(vehicle);
}
void ClientNetworkHandler::onEntityStatus(const EntityStatusS2CPacket& packet) {
  Entity* entity = getEntity(packet.id);
  if(entity != nullptr) {
    entity->processServerEntityStatus(packet.status);
  }
}
void ClientNetworkHandler::onEntityEquipmentUpdate(const EntityEquipmentUpdateS2CPacket& packet) {
  Entity* entity = getEntity(packet.id);
  if(entity != nullptr) {
    entity->setEquipmentStack(packet.slot, packet.itemRawId, packet.itemDamage);
  }
}
} // namespace net::minecraft::client::multiplayer
