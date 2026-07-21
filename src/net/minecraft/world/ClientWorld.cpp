#include "net/minecraft/world/ClientWorld.hpp"
#include <algorithm>
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/events/GameEventListener.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"
namespace net::minecraft {
namespace {
WorldStorage* multiplayerWorldStorage() {
 // Client worlds need a save handler object during World construction, but a
 // derived member is not alive yet when the base constructor runs.
 static EmptyWorldStorage storage;
 return &storage;
}
} // namespace
ChunkSource* ClientWorld::createChunkCache() {
 auto cache = std::make_unique<world::chunk::ChunkCache>(this, nullptr, nullptr);
 ChunkSource* ptr = cache.get();
 setChunkCache(std::move(cache));
 return ptr;
}
ClientWorld::ClientWorld(client::multiplayer::ClientNetworkHandler* networkHandler, std::uint64_t seed, int dimensionId)
    : World(multiplayerWorldStorage(), "MpServer", static_cast<std::int64_t>(seed), true),
      networkHandler_(networkHandler) {
 dimension = Dimension::fromId(dimensionId);
 if(dimension != nullptr) {
  dimension->setWorld(this);
 }
 isRemote_ = true;
 setSpawnPos(Vec3i{8, 64, 8});
 updateSkyBrightness();
 prepareWeather();
}
void ClientWorld::tick() {
  mod::WorldTickEvent beforeTick{this, true, true};
mod::lua::setModContext(this, true);
  net::minecraft::mod::runtime::luaHookWorldTick(beforeTick);
  mod::lua::clearModContext();
 setTime(time() + 1);
 updateSkyBrightness();
 for(int i = 0; i < 10 && !pendingEntities_.empty(); ++i) {
  auto it = pendingEntities_.begin();
  Entity* entity = *it;
  pendingEntities_.erase(it);
  if(std::find(entities_.begin(), entities_.end(), entity) == entities_.end()) {
   spawnEntity(entity);
  }
 }
 if(networkHandler_ != nullptr) {
  networkHandler_->tick();
 }
 for(auto it = blockResets_.begin(); it != blockResets_.end();) {
  if(--it->delay != 0) {
   ++it;
   continue;
  }
  World::setBlockWithoutNotifyingNeighbors(it->x, it->y, it->z, it->block, it->meta);
  blockUpdateEvent(it->x, it->y, it->z);
  it = blockResets_.erase(it);
 }
  mod::WorldTickEvent afterTick{this, true, false};
mod::lua::setModContext(this, true);
  net::minecraft::mod::runtime::luaHookWorldTick(afterTick);
  mod::lua::clearModContext();
}
void ClientWorld::clearBlockResets(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 for(auto it = blockResets_.begin(); it != blockResets_.end();) {
  if(it->x >= minX && it->y >= minY && it->z >= minZ && it->x <= maxX && it->y <= maxY && it->z <= maxZ) {
   it = blockResets_.erase(it);
  } else {
   ++it;
  }
 }
}
void ClientWorld::updateChunk(int chunkX, int chunkZ, bool load) {
 auto* cache = dynamic_cast<world::chunk::ChunkCache*>(getChunkSource());
 if(cache == nullptr) {
  return;
 }
 if(load) {
  cache->loadChunk(chunkX, chunkZ);
 } else {
  cache->unloadChunk(chunkX, chunkZ);
  setBlocksDirty(chunkX * 16, 0, chunkZ * 16, chunkX * 16 + 15, Chunk::height, chunkZ * 16 + 15);
 }
}
bool ClientWorld::spawnEntity(Entity* entity) {
 const bool spawned = World::spawnEntity(entity);
 if(entity != nullptr) {
  forcedEntities_.insert(entity);
  if(!spawned) {
   pendingEntities_.insert(entity);
  }
 }
 return spawned;
}
void ClientWorld::remove(Entity* entity) {
 World::remove(entity);
 if(entity != nullptr) {
  forcedEntities_.erase(entity);
 }
}
void ClientWorld::notifyEntityAdded(Entity* entity) {
 World::notifyEntityAdded(entity);
 if(entity != nullptr) {
  pendingEntities_.erase(entity);
 }
}
void ClientWorld::notifyEntityRemoved(Entity* entity) {
 World::notifyEntityRemoved(entity);
 if(entity != nullptr && forcedEntities_.contains(entity)) {
  pendingEntities_.insert(entity);
 }
}
void ClientWorld::forceEntity(int id, Entity* entity) {
 if(Entity* existing = getEntity(id)) {
  remove(existing);
 }
 if(entity == nullptr) {
  return;
 }
 forcedEntities_.insert(entity);
 entity->id = id;
 if(!spawnEntity(entity)) {
  pendingEntities_.insert(entity);
 }
 entitiesByNetworkId_[id] = entity;
}
Entity* ClientWorld::getEntity(int id) {
 const auto it = entitiesByNetworkId_.find(id);
 return it == entitiesByNetworkId_.end() ? nullptr : it->second;
}
Entity* ClientWorld::removeEntity(int id) {
 const auto it = entitiesByNetworkId_.find(id);
 if(it == entitiesByNetworkId_.end()) {
  return nullptr;
 }
 Entity* entity = it->second;
 entitiesByNetworkId_.erase(it);
 if(entity != nullptr) {
  forcedEntities_.erase(entity);
  remove(entity);
 }
 return entity;
}
void ClientWorld::addBlockReset(int x, int y, int z, int block, int meta) {
 blockResets_.emplace_back(x, y, z, block, meta);
}
bool ClientWorld::setBlockMetaWithoutNotifyingNeighbors(int x, int y, int z, int meta) {
 const int previousId = getBlockId(x, y, z);
 const int previousMeta = getBlockMeta(x, y, z);
 if(!World::setBlockMetaWithoutNotifyingNeighbors(x, y, z, meta)) {
  return false;
 }
 addBlockReset(x, y, z, previousId, previousMeta);
 return true;
}
bool ClientWorld::setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId, int meta) {
 const int previousId = getBlockId(x, y, z);
 const int previousMeta = getBlockMeta(x, y, z);
 if(!World::setBlockWithoutNotifyingNeighbors(x, y, z, blockId, meta)) {
  return false;
 }
 addBlockReset(x, y, z, previousId, previousMeta);
 return true;
}
bool ClientWorld::setBlockWithMetaFromPacket(int x, int y, int z, int blockId, int meta) {
 clearBlockResets(x, y, z, x, y, z);
 if(!World::setBlockWithoutNotifyingNeighbors(x, y, z, blockId, meta)) {
  return false;
 }
 blockUpdate(x, y, z, blockId);
 return true;
}
void ClientWorld::updateWeatherCycles() {
  mod::WeatherCycleEvent event{this, true, false};
  if(event.canceled) {
  return;
 }
 if(dimension != nullptr && dimension->hasCeiling) {
  return;
 }
 if(weather_.ticksSinceLightning > 0) {
  --weather_.ticksSinceLightning;
 }
 if(!hasStorageBackedProperties_) {
  return;
 }
 weather_.tickGradients(properties_.getRaining(), properties_.getThundering());
}
void ClientWorld::disconnect() {
 if(networkHandler_ != nullptr) {
  networkHandler_->disconnect("Quitting");
 }
}
} // namespace net::minecraft
