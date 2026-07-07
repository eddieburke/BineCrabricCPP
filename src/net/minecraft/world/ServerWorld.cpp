#include "net/minecraft/world/ServerWorld.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/WaterCreatureEntity.hpp"
#include "net/minecraft/entity/passive/AnimalEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/network/packet/BlockPackets.hpp"
#include "net/minecraft/network/packet/EntityPackets.hpp"
#include "net/minecraft/network/packet/WorldPackets.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/server/world/chunk/ServerChunkCache.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/explosion/Explosion.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
#include <algorithm>
#include <cmath>
namespace net::minecraft {
ServerWorld::ServerWorld(
    server::MinecraftServer* server,
    WorldStorage* storage,
    const std::string& name,
    int dimensionId,
    std::uint64_t seed)
    : World(storage, name, static_cast<std::int64_t>(seed), true) {
  server_ = server;
  isRemote_ = false;
  if(dimension != nullptr && dimension->id != dimensionId) {
    dimension = Dimension::fromId(dimensionId);
    if(dimension != nullptr) {
      dimension->setWorld(this);
    }
  }
  createChunkCache();
}
void ServerWorld::updateEntity(Entity* entity, bool requireLoaded) {
  if(entity == nullptr) {
    return;
  }
  if(server_ != nullptr && !server_->spawnAnimals) {
    if(dynamic_cast<entity::passive::AnimalEntity*>(entity) != nullptr || dynamic_cast<entity::WaterCreatureEntity*>(entity) != nullptr) {
      entity->markDead();
    }
  }
  if(entity->passenger == nullptr || dynamic_cast<PlayerEntity*>(entity->passenger) == nullptr) {
    World::updateEntity(entity, requireLoaded);
  }
}
void ServerWorld::tickVehicle(Entity* vehicle, bool requireLoaded) {
  World::updateEntity(vehicle, requireLoaded);
}
ChunkSource* ServerWorld::createChunkCache() {
  WorldStorage* storage = getDimensionData();
  if(storage == nullptr || dimension == nullptr) {
    return getChunkSource();
  }
  std::unique_ptr<ChunkStorage> chunkStorage = storage->getChunkStorage(dimension.get());
  chunkGeneratorSource_ = dimension->createChunkGenerator();
  auto cache = std::make_unique<server::world::chunk::ServerChunkCache>(this, std::move(chunkStorage), chunkGeneratorSource_.get());
  chunkCache = cache.get();
  setChunkCache(std::move(cache));
  return chunkCache;
}
std::vector<block::entity::BlockEntity*> ServerWorld::getBlockEntities(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
  std::vector<block::entity::BlockEntity*> result;
  for(block::entity::BlockEntity* blockEntity : blockEntities) {
    if(blockEntity == nullptr) {
      continue;
    }
    if(blockEntity->x < minX || blockEntity->y < minY || blockEntity->z < minZ || blockEntity->x >= maxX || blockEntity->y >= maxY || blockEntity->z >= maxZ) {
      continue;
    }
    result.push_back(blockEntity);
  }
  return result;
}
bool ServerWorld::canInteract(PlayerEntity* player, int x, int y, int z) {
  (void)y;
  if(bypassSpawnProtection) {
    return true;
  }
  if(player == nullptr) {
    return false;
  }
  const int spawnX = getProperties().getSpawnX();
  const int spawnZ = getProperties().getSpawnZ();
  int dx = static_cast<int>(MathHelper::abs(x - spawnX));
  int dz = static_cast<int>(MathHelper::abs(z - spawnZ));
  const int distance = std::max(dx, dz);
  return distance > 16 || (server_ != nullptr && server_->isOperator(player->name));
}
void ServerWorld::notifyEntityAdded(Entity* entity) {
  World::notifyEntityAdded(entity);
  if(entity != nullptr) {
    entitiesById_.put(entity->id, entity);
  }
}
void ServerWorld::notifyEntityRemoved(Entity* entity) {
  World::notifyEntityRemoved(entity);
  if(entity != nullptr) {
    entitiesById_.remove(entity->id);
  }
}
Entity* ServerWorld::getEntity(int id) {
  return entitiesById_.get(id);
}
bool ServerWorld::spawnGlobalEntity(Entity* entity) {
  if(!World::spawnGlobalEntity(entity)) {
    return false;
  }
  if(server_ != nullptr && dimension != nullptr && entity != nullptr) {
    GlobalEntitySpawnS2CPacket packet;
    packet.id = entity->id;
    packet.type = 1;
    packet.x = static_cast<int>(entity->x);
    packet.y = static_cast<int>(entity->y);
    packet.z = static_cast<int>(entity->z);
    server_->playerManager.sendToAround(entity->x, entity->y, entity->z, 512.0, dimension->id, packet);
  }
  return true;
}
void ServerWorld::broadcastEntityEvent(Entity* entity, std::uint8_t event) {
  if(server_ == nullptr || dimension == nullptr || entity == nullptr) {
    return;
  }
  EntityStatusS2CPacket packet;
  packet.id = entity->id;
  packet.status = static_cast<std::int8_t>(event);
  server_->getEntityTracker(dimension->id).sendToAround(entity, packet);
}
Explosion ServerWorld::createExplosion(Entity* source, double x, double y, double z, float power, bool fire) {
  Explosion explosion(this, source, x, y, z, power);
  explosion.fire = fire;
  explosion.explode();
  explosion.playExplosionSound(false);
  if(server_ != nullptr && dimension != nullptr) {
    ExplosionS2CPacket packet;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.radius = power;
    packet.affectedBlocks.assign(explosion.damagedBlocks.begin(), explosion.damagedBlocks.end());
    server_->playerManager.sendToAround(x, y, z, 64.0, dimension->id, packet);
  }
  return explosion;
}
void ServerWorld::playNoteBlockActionAt(int x, int y, int z, int soundType, int pitch) {
  World::playNoteBlockActionAt(x, y, z, soundType, pitch);
  if(server_ != nullptr && dimension != nullptr) {
    PlayNoteSoundS2CPacket packet;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.instrument = soundType;
    packet.pitch = pitch;
    server_->playerManager.sendToAround(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), 64.0,
                                        dimension->id, packet);
  }
}
void ServerWorld::forceSave() {
  if(getDimensionData() != nullptr) {
    getDimensionData()->forceSave();
  }
}
void ServerWorld::updateWeatherCycles() {
  const bool wasRaining = isRaining();
  World::updateWeatherCycles();
  if(server_ == nullptr || wasRaining == isRaining()) {
    return;
  }
  if(wasRaining) {
    GameStateChangeS2CPacket packet;
    packet.reason = 2;
    server_->playerManager.sendToAll(packet);
  } else {
    GameStateChangeS2CPacket packet;
    packet.reason = 1;
    server_->playerManager.sendToAll(packet);
  }
}
void ServerWorld::saveWithLoadingDisplay(bool saveEntities, client::gui::screen::LoadingDisplay* display) {
  save(true);
  if(getChunkSource() != nullptr) {
    getChunkSource()->save(saveEntities, display);
  }
  forceSave();
}
} // namespace net::minecraft
