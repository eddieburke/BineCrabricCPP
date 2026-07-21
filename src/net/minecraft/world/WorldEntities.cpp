#include <algorithm>
#include <cmath>
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/util/logging/Log.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/LuaBlockEntityBindings.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
using net::minecraft::util::logging::Log;
using net::minecraft::util::logging::LogLevel;
namespace net::minecraft {
namespace {
bool isInvalidDouble(double value) {
 return std::isnan(value) || std::isinf(value);
}
} // namespace
std::vector<Entity*> World::getEntities(Entity* except, const Box& box) {
 std::vector<Entity*> result;
 for(Entity* entity : entities_) {
  if(entity == nullptr || entity == except || entity->dead) {
   continue;
  }
  if(entity->boundingBox.intersects(box)) {
   result.push_back(entity);
  }
 }
 return result;
}
void World::updateEntityLists() {
 entities_.erase(std::remove_if(entities_.begin(),
                                entities_.end(),
                                [this](Entity* entity) {
                                 return entity != nullptr &&
                                        std::find(entitiesToUnload_.begin(), entitiesToUnload_.end(), entity) !=
                                            entitiesToUnload_.end();
                                }),
                 entities_.end());
 for(Entity* entity : entitiesToUnload_) {
  if(entity == nullptr) {
   continue;
  }
  if(entity->isPersistent && hasChunk(entity->chunkX, entity->chunkZ)) {
   getChunk(entity->chunkX, entity->chunkZ).removeEntity(entity);
  }
 }
 for(Entity* entity : entitiesToUnload_) {
  notifyEntityRemoved(entity);
 }
 entitiesToUnload_.clear();
 for(std::size_t i = 0; i < entities_.size(); ++i) {
  Entity* entity = entities_[i];
  if(entity == nullptr) {
   continue;
  }
  if(entity->vehicle != nullptr) {
   if(!entity->vehicle->dead && entity->vehicle->passenger == entity) {
    continue;
   }
   entity->vehicle->passenger = nullptr;
   entity->vehicle = nullptr;
  }
  if(!entity->dead) {
   continue;
  }
  if(entity->isPersistent && hasChunk(entity->chunkX, entity->chunkZ)) {
   getChunk(entity->chunkX, entity->chunkZ).removeEntity(entity);
  }
  entities_.erase(entities_.begin() + static_cast<std::ptrdiff_t>(i));
  notifyEntityRemoved(entity);
  --i;
 }
}
bool World::spawnGlobalEntity(Entity* entity) {
 if(entity == nullptr) {
  return false;
 }
 if(std::find(globalEntities.begin(), globalEntities.end(), entity) != globalEntities.end()) {
  return true;
 }
 entity->world = this;
 globalEntities.push_back(entity);
 notifyEntityAdded(entity);
 return true;
}
bool World::spawnEntity(Entity* entity) {
 if(entity == nullptr) {
  return false;
 }
 if(std::find(entities_.begin(), entities_.end(), entity) != entities_.end()) {
  return true;
 }
 const int chunkX = MathHelper::floor(entity->x / 16.0);
 const int chunkZ = MathHelper::floor(entity->z / 16.0);
 const bool isPlayer = dynamic_cast<PlayerEntity*>(entity) != nullptr;
 if(!isPlayer && !hasChunk(chunkX, chunkZ)) {
  return false;
 }
 if(isPlayer) {
  players.push_back(static_cast<PlayerEntity*>(entity));
  updateSleepingPlayers();
 }
 entity->world = this;
 entities_.push_back(entity);
 getChunk(chunkX, chunkZ).addEntity(entity);
 notifyEntityAdded(entity);
 return true;
}
void World::notifyEntityAdded(Entity* entity) {
 events_.notifyEntityAdded(entity);
   if(entity != nullptr && net::minecraft::mod::runtime::hasLuaHook(net::minecraft::mod::runtime::LuaEventId::EntitySpawn)) {
   mod::EntitySpawnEvent event;
   event.entity = entity;
   event.entityId = entity->id;
   event.entityType = entity::EntityRegistry::getId(*entity);
   net::minecraft::mod::runtime::luaHookEntitySpawn(event);
  }
}
void World::notifyEntityRemoved(Entity* entity) {
 events_.notifyEntityRemoved(entity);
   if(entity != nullptr && net::minecraft::mod::runtime::hasLuaHook(net::minecraft::mod::runtime::LuaEventId::EntityRemove)) {
   mod::EntityRemoveEvent event;
   event.entity = entity;
   event.entityId = entity->id;
   event.entityType = entity::EntityRegistry::getId(*entity);
   net::minecraft::mod::runtime::luaHookEntityRemove(event);
  }
}
void World::serverRemove(Entity* entity) {
 if(entity == nullptr) {
  return;
 }
 entity->markDead();
 entities_.erase(std::remove(entities_.begin(), entities_.end(), entity), entities_.end());
 if(auto* player = dynamic_cast<PlayerEntity*>(entity)) {
  players.erase(std::remove(players.begin(), players.end(), player), players.end());
  updateSleepingPlayers();
 }
 if(entity->isPersistent) {
  if(Chunk* chunk = getChunkIfLoaded(entity->chunkX << 4, entity->chunkZ << 4)) {
   chunk->removeEntity(entity);
  }
 }
 notifyEntityRemoved(entity);
}
void World::remove(Entity* entity) {
 if(entity == nullptr) {
  return;
 }
 if(entity->passenger != nullptr) {
  entity->passenger->setVehicle(nullptr);
 }
 if(entity->vehicle != nullptr) {
  entity->setVehicle(nullptr);
 }
 entity->markDead();
 entities_.erase(std::remove(entities_.begin(), entities_.end(), entity), entities_.end());
 if(auto* player = dynamic_cast<PlayerEntity*>(entity)) {
  players.erase(std::remove(players.begin(), players.end(), player), players.end());
  updateSleepingPlayers();
 }
 if(entity->isPersistent) {
  if(Chunk* chunk = getChunkIfLoaded(entity->chunkX << 4, entity->chunkZ << 4)) {
   chunk->removeEntity(entity);
  }
 }
}
void World::notifyEntityPickup(Entity* entity, PlayerEntity* collector) {
 events_.notifyEntityPickup(entity, collector);
}
int World::countEntities(const std::string& entityType) const {
 int count = 0;
 for(Entity* entity : entities_) {
  if(entity == nullptr || entity->dead) {
   continue;
  }
  if(*reinterpret_cast<const void* const*>(entity) == nullptr) {
   continue;
  }
  if(EntityRegistry::getId(*entity) == entityType) {
   ++count;
  }
 }
 return count;
}
bool World::spawnMob(entity::LivingEntity* mob) {
 if(mob == nullptr) {
  return false;
 }
 mob->world = this;
 mob->initializeSpawnState(random_);
 return spawnEntity(mob);
}
entity::LivingEntity* World::spawnMob(const std::string& entityType,
                                      const std::function<bool(entity::LivingEntity&)>& setup) {
 std::unique_ptr<Entity> entity = EntityRegistry::create(entityType, this);
 auto* mob = dynamic_cast<entity::LivingEntity*>(entity.get());
 if(mob == nullptr) {
  return nullptr;
 }
 if(setup && !setup(*mob)) {
  return nullptr;
 }
 if(!spawnMob(mob)) {
  return nullptr;
 }
 entity.release();
 return mob;
}
entity::LivingEntity* World::spawnMob(
    const std::string& entityType, double x, double y, double z, float yaw, float pitch) {
 return spawnMob(entityType, [=](entity::LivingEntity& mob) {
  mob.setPositionAndAnglesKeepPrevAngles(x, y, z, yaw, pitch);
  return true;
 });
}
void World::addPlayer(PlayerEntity* player) {
 if(player == nullptr) {
  return;
 }
  try {
   if(const NbtCompound* playerNbt = properties_.getPlayerNbt(); playerNbt != nullptr) {
    player->readNbt(*playerNbt);
    properties_.clearPlayerNbt();
   }
   setChunkCacheCenterFromBlockPos(MathHelper::floor(player->x), MathHelper::floor(player->z));
   spawnEntity(player);
  } catch(...) {
   Log::LOGGER.log(LogLevel::Warning, "WorldEntities: addPlayer failed, player spawn silently discarded");
  }
}
void World::updateEntity(Entity* entity, bool requireLoaded, int depth) {
 if(entity == nullptr) {
  return;
 }
 const int floorX = MathHelper::floor(entity->x);
 const int floorZ = MathHelper::floor(entity->z);
 constexpr int regionRadius = 32;
 if(requireLoaded && !isRegionLoaded(floorX - regionRadius,
                                     0,
                                     floorZ - regionRadius,
                                     floorX + regionRadius,
                                     Chunk::height,
                                     floorZ + regionRadius)) {
  return;
 }
 entity->lastTickX = entity->x;
 entity->lastTickY = entity->y;
 entity->lastTickZ = entity->z;
 entity->prevYaw = entity->yaw;
 entity->prevPitch = entity->pitch;
 if(requireLoaded && entity->isPersistent) {
  if(entity->vehicle != nullptr) {
   entity->tickRiding();
  } else {
   entity->tick();
  }
 }
 if(isInvalidDouble(entity->x)) {
  entity->x = entity->lastTickX;
 }
 if(isInvalidDouble(entity->y)) {
  entity->y = entity->lastTickY;
 }
 if(isInvalidDouble(entity->z)) {
  entity->z = entity->lastTickZ;
 }
 if(isInvalidDouble(entity->pitch)) {
  entity->pitch = entity->prevPitch;
 }
 if(isInvalidDouble(entity->yaw)) {
  entity->yaw = entity->prevYaw;
 }
 const int chunkX = MathHelper::floor(entity->x / 16.0);
 const int chunkSlice = MathHelper::floor(entity->y / 16.0);
 const int chunkZ = MathHelper::floor(entity->z / 16.0);
 if(!entity->isPersistent || entity->chunkX != chunkX || entity->chunkSlice != chunkSlice ||
    entity->chunkZ != chunkZ) {
  if(entity->isPersistent && hasChunk(entity->chunkX, entity->chunkZ)) {
   getChunk(entity->chunkX, entity->chunkZ).removeEntity(entity, entity->chunkSlice);
  }
  if(hasChunk(chunkX, chunkZ)) {
   entity->isPersistent = true;
   getChunk(chunkX, chunkZ).addEntity(entity);
  } else {
   entity->isPersistent = false;
  }
 }
 if(requireLoaded && entity->isPersistent && entity->passenger != nullptr) {
  if(entity->passenger->dead || entity->passenger->vehicle != entity) {
   entity->passenger->vehicle = nullptr;
   entity->passenger = nullptr;
  } else if(depth < 8) {
   updateEntity(entity->passenger, requireLoaded, depth + 1);
  } else {
   entity->passenger->vehicle = nullptr;
   entity->passenger = nullptr;
  }
 }
}
void World::addEntities(std::vector<Entity*>& entities) {
 for(Entity* entity : entities) {
  if(entity != nullptr) {
   entity->world = this;
  }
 }
 entities_.insert(entities_.end(), entities.begin(), entities.end());
 for(Entity* entity : entities) {
  if(entity != nullptr) {
   notifyEntityAdded(entity);
  }
 }
}
void World::unloadEntities(std::vector<Entity*>& entities) {
 entitiesToUnload_.insert(entitiesToUnload_.end(), entities.begin(), entities.end());
}
void World::processBlockUpdates(const std::vector<block::entity::BlockEntity*>& blockUpdates) {
 if(processingDeferred_) {
  blockEntityUpdateQueue_.insert(blockEntityUpdateQueue_.end(), blockUpdates.begin(), blockUpdates.end());
 } else {
  for(block::entity::BlockEntity* blockEntity : blockUpdates) {
   if(blockEntity == nullptr || blockEntity->isRemoved()) {
    continue;
   }
   if(std::find(blockEntities.begin(), blockEntities.end(), blockEntity) == blockEntities.end()) {
    blockEntities.push_back(blockEntity);
   }
  }
 }
}
void World::updateSleepingPlayers() {
 allPlayersSleeping = !players.empty();
 for(PlayerEntity* player : players) {
  if(player == nullptr || player->isSleeping()) {
   continue;
  }
  allPlayersSleeping = false;
  break;
 }
}
bool World::canSkipNight() {
 if(!allPlayersSleeping || isRemote_) {
  return false;
 }
 for(PlayerEntity* player : players) {
  if(player == nullptr || player->isFullyAsleep()) {
   continue;
  }
  return false;
 }
 return true;
}
void World::afterSkipNight() {
 allPlayersSleeping = false;
 for(PlayerEntity* player : players) {
  if(player == nullptr || !player->isSleeping()) {
   continue;
  }
  player->wakeUp(false, false, true);
 }
 clearWeather();
}
void World::tickEntities() {
 for(std::size_t i = 0; i < globalEntities.size(); ++i) {
  Entity* entity = globalEntities[i];
  if(entity == nullptr) {
   continue;
  }
  bool canceled = false;
    if(net::minecraft::mod::runtime::hasLuaHook(net::minecraft::mod::runtime::LuaEventId::EntityTick)) {
    mod::EntityTickEvent event;
    event.world = this;
    event.entity = entity;
    event.remote = isRemote();
    net::minecraft::mod::runtime::luaHookEntityTick(event);
    canceled = event.canceled;
   }
   if(!canceled) {
    entity->tick();
   }
  if(!entity->dead) {
   continue;
  }
  globalEntities.erase(globalEntities.begin() + static_cast<std::ptrdiff_t>(i));
  --i;
 }
 entities_.erase(std::remove_if(entities_.begin(),
                                entities_.end(),
                                [this](Entity* entity) {
                                 return entity != nullptr &&
                                        std::find(entitiesToUnload_.begin(), entitiesToUnload_.end(), entity) !=
                                            entitiesToUnload_.end();
                                }),
                 entities_.end());
 for(Entity* entity : entitiesToUnload_) {
  if(entity == nullptr) {
   continue;
  }
  if(entity->isPersistent && hasChunk(entity->chunkX, entity->chunkZ)) {
   getChunk(entity->chunkX, entity->chunkZ).removeEntity(entity);
  }
 }
 for(Entity* entity : entitiesToUnload_) {
  notifyEntityRemoved(entity);
 }
 entitiesToUnload_.clear();
 for(std::size_t i = 0; i < entities_.size(); ++i) {
  Entity* entity = entities_[i];
  if(entity == nullptr) {
   continue;
  }
  if(entity->vehicle != nullptr) {
   if(!entity->vehicle->dead && entity->vehicle->passenger == entity) {
    continue;
   }
   entity->vehicle->passenger = nullptr;
   entity->vehicle = nullptr;
  }
  if(!entity->dead) {
   bool canceled = false;
    if(net::minecraft::mod::runtime::hasLuaHook(net::minecraft::mod::runtime::LuaEventId::EntityTick)) {
    mod::EntityTickEvent event;
    event.world = this;
    event.entity = entity;
    event.remote = isRemote();
    net::minecraft::mod::runtime::luaHookEntityTick(event);
    canceled = event.canceled;
   }
   if(!canceled) {
    updateEntity(entity);
   }
  }
  if(!entity->dead) {
   continue;
  }
  if(entity->isPersistent && hasChunk(entity->chunkX, entity->chunkZ)) {
   getChunk(entity->chunkX, entity->chunkZ).removeEntity(entity);
  }
  if(auto* deadPlayer = dynamic_cast<PlayerEntity*>(entity)) {
   players.erase(std::remove(players.begin(), players.end(), deadPlayer), players.end());
   updateSleepingPlayers();
  }
  entities_.erase(entities_.begin() + static_cast<std::ptrdiff_t>(i));
  notifyEntityRemoved(entity);
  --i;
 }
 processingDeferred_ = true;
 for(std::size_t i = 0; i < blockEntities.size(); ++i) {
  block::entity::BlockEntity* blockEntity = blockEntities[i];
  if(blockEntity == nullptr) {
   continue;
  }
  if(!blockEntity->isRemoved()) {
   bool canceled = false;
    if(net::minecraft::mod::runtime::hasLuaHook(net::minecraft::mod::runtime::LuaEventId::TileEntityTick)) {
    mod::TileEntityTickEvent tileEvent;
    tileEvent.world = this;
    tileEvent.entity = blockEntity;
    tileEvent.remote = isRemote();
    tileEvent.canceled = false;
    net::minecraft::mod::runtime::luaHookTileEntityTick(tileEvent);
    canceled = tileEvent.canceled;
   }
   if(!canceled) {
    blockEntity->tick();
   }
  }
  if(!blockEntity->isRemoved()) {
   continue;
  }
  blockEntities.erase(blockEntities.begin() + static_cast<std::ptrdiff_t>(i));
  mod::runtime::clearTileEntityAnimation(blockEntity);
  --i;
  Chunk& chunk = getChunk(blockEntity->x >> 4, blockEntity->z >> 4);
  chunk.removeBlockEntityAt(blockEntity->x & 0xF, blockEntity->y, blockEntity->z & 0xF);
 }
 processingDeferred_ = false;
 if(!blockEntityUpdateQueue_.empty()) {
  for(block::entity::BlockEntity* blockEntity : blockEntityUpdateQueue_) {
   if(blockEntity == nullptr || blockEntity->isRemoved()) {
    continue;
   }
   if(std::find(blockEntities.begin(), blockEntities.end(), blockEntity) == blockEntities.end()) {
    blockEntities.push_back(blockEntity);
   }
   blockUpdateEvent(blockEntity->x, blockEntity->y, blockEntity->z);
  }
  blockEntityUpdateQueue_.clear();
 }
}
void World::loadChunksNearEntity(Entity* entity) {
 if(entity == nullptr) {
  return;
 }
 const int chunkX = MathHelper::floor(entity->x / 16.0);
 const int chunkZ = MathHelper::floor(entity->z / 16.0);
 if(chunkCache_ != nullptr) {
  chunkCache_->setActiveRadius(chunkResidentRadiusChunks_);
  chunkCache_->prefetchChunksNear(chunkX, chunkZ);
 } else {
  const int radius = chunkResidentRadiusChunks_;
  constexpr int kMaxPreloadsPerCall = 8;
  int preloaded = 0;
  for(int ring = 0; ring <= radius && preloaded < kMaxPreloadsPerCall; ++ring) {
   for(int dx = -ring; dx <= ring && preloaded < kMaxPreloadsPerCall; ++dx) {
    for(int dz = -ring; dz <= ring && preloaded < kMaxPreloadsPerCall; ++dz) {
     if(std::max(std::abs(dx), std::abs(dz)) != ring) {
      continue;
     }
     if(hasChunk(chunkX + dx, chunkZ + dz)) {
      continue;
     }
     [[maybe_unused]] Chunk& chunk = getChunk(chunkX + dx, chunkZ + dz);
     ++preloaded;
    }
   }
  }
 }
 if(std::find(entities_.begin(), entities_.end(), entity) == entities_.end()) {
  spawnEntity(entity);
 }
}
PlayerEntity* World::getClosestPlayer(double targetX, double targetY, double targetZ, double range) {
 double bestDistance = -1.0;
 PlayerEntity* closest = nullptr;
 const double rangeSq = range < 0.0 ? -1.0 : range * range;
 for(PlayerEntity* player : players) {
  if(player == nullptr) {
   continue;
  }
  const double distanceSq = player->getSquaredDistance(targetX, targetY, targetZ);
  if(rangeSq >= 0.0 && distanceSq >= rangeSq) {
   continue;
  }
  if(bestDistance >= 0.0 && distanceSq >= bestDistance) {
   continue;
  }
  bestDistance = distanceSq;
  closest = player;
 }
 return closest;
}
PlayerEntity* World::getClosestPlayer(Entity* entity, double range) {
 if(entity == nullptr) {
  return nullptr;
 }
 return getClosestPlayer(entity->x, entity->y, entity->z, range);
}
Entity* World::getClosestPlayerEntity(Entity* entity, double range) {
 return getClosestPlayer(entity, range);
}
PlayerEntity* World::getPlayer(const std::string& name) {
 for(PlayerEntity* player : players) {
  if(player != nullptr && player->name == name) {
   return player;
  }
 }
 return nullptr;
}
} // namespace net::minecraft
