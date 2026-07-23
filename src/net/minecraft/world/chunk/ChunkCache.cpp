#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <thread>
#include "net/minecraft/util/logging/Log.hpp"
#include "net/minecraft/world/World.hpp"
using net::minecraft::util::logging::Log;
using net::minecraft::util::logging::LogLevel;
namespace net::minecraft::world::chunk {
ChunkCache::ChunkCache(World* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator)
    : empty_(world, 0, 0), world_(world), storage_(std::move(storage)), generator_(generator) {
}
bool ChunkCache::isChunkLoaded(int chunkX, int chunkZ) const {
 return chunksByPos_.find(ChunkPos{chunkX, chunkZ}) != chunksByPos_.end();
}
bool ChunkCache::isChunkDataReady(int chunkX, int chunkZ) const {
 const auto it = chunksByPos_.find(ChunkPos{chunkX, chunkZ});
 return it != chunksByPos_.end() && it->second != nullptr && it->second->dataReady;
}
void ChunkCache::markChunkDataReady(int chunkX, int chunkZ) {
 const auto it = chunksByPos_.find(ChunkPos{chunkX, chunkZ});
 if(it != chunksByPos_.end() && it->second != nullptr) {
  it->second->dataReady = true;
 }
}
void ChunkCache::dropChunk(int chunkX, int chunkZ) {
 if(world_ == nullptr || (generator_ == nullptr && storage_ == nullptr)) {
  return;
 }
 const Vec3i spawnPos = world_->getSpawnPos();
 const int dx = chunkX * 16 + 8 - spawnPos.x;
 const int dz = chunkZ * 16 + 8 - spawnPos.z;
 constexpr int radius = 128;
 if(dx < -radius || dx > radius || dz < -radius || dz > radius) {
  chunksToUnload_.insert(ChunkPos{chunkX, chunkZ});
 }
}
void ChunkCache::retireFromLighting(Chunk* chunk) {
 if(chunk == nullptr || chunk->isEmpty()) {
  return;
 }
 while(!chunk->beginRenderEviction()) {
  std::this_thread::yield();
 }
}
void ChunkCache::unloadChunk(int chunkX, int chunkZ) {
 const ChunkPos pos{chunkX, chunkZ};
 pendingLoads_.erase(pos);
 auto it = chunksByPos_.find(pos);
 if(it == chunksByPos_.end()) {
  return;
 }
 Chunk* chunk = it->second;
 if(chunk != nullptr && !chunk->isEmpty()) {
  retireFromLighting(chunk);
  chunk->unload();
 }
 chunks_.erase(std::remove(chunks_.begin(), chunks_.end(), chunk), chunks_.end());
 chunksByPos_.erase(it);
 ownedChunks_.erase(chunk);
}
Chunk& ChunkCache::loadChunk(int chunkX, int chunkZ) {
 const ChunkPos pos{chunkX, chunkZ};
 chunksToUnload_.erase(pos);
 const auto existing = chunksByPos_.find(pos);
 if(existing != chunksByPos_.end()) {
  return *existing->second;
 }
 const auto pendingIt = pendingLoads_.find(pos);
 if(pendingIt != pendingLoads_.end()) {
  // Demanded before the background load finished. If it is done, adopt the
  // result; otherwise drop the pending entry and produce synchronously (the
  // worker's late result is discarded). Never wait on the worker here: the
  // caller may already hold ioMutex_ (decoration), and waiting would deadlock.
  const std::shared_ptr<PendingLoad> pending = pendingIt->second;
  pendingLoads_.erase(pendingIt);
  if(pending->done.load(std::memory_order_acquire) && pending->chunk != nullptr) {
   return adoptChunk(chunkX, chunkZ, std::move(pending->chunk));
  }
 }
 if(generator_ == nullptr && storage_ == nullptr) {
  // Multiplayer mode: instantiate new blank chunk
  auto generated = std::make_unique<Chunk>(world_, chunkX, chunkZ);
  std::fill(generated->skyLight.bytes.begin(), generated->skyLight.bytes.end(), static_cast<std::uint8_t>(0xFF));
  generated->loaded = true;
  generated->dataReady = false;
  Chunk* chunk = generated.get();
  ownedChunks_.emplace(chunk, std::move(generated));
  chunksByPos_[pos] = chunk;
  chunks_.push_back(chunk);
  world_->registerChunkForLighting(chunk);
  world_->setBlocksDirty(chunkX * 16, 0, chunkZ * 16, chunkX * 16 + 15, Chunk::height - 1, chunkZ * 16 + 15);
  return *chunk;
 }
 return adoptChunk(chunkX, chunkZ, produceChunk(chunkX, chunkZ));
}
std::unique_ptr<Chunk> ChunkCache::produceChunk(int chunkX, int chunkZ) {
 const std::lock_guard lock(ioMutex_);
 if(storage_ != nullptr && world_ != nullptr) {
  try {
   auto loaded = std::make_unique<Chunk>(std::move(storage_->loadChunk(world_, chunkX, chunkZ)));
   if(!loaded->empty && loaded->chunkPosEquals(chunkX, chunkZ)) {
    return loaded;
   }
  } catch(...) {
   Log::LOGGER.log(LogLevel::Warning,
                   "ChunkCache: chunk load failed for (" + std::to_string(chunkX) + ", " +
                       std::to_string(chunkZ) + "), falling back to generation");
  }
 }
 if(generator_ != nullptr) {
  return std::make_unique<Chunk>(std::move(generator_->getChunk(chunkX, chunkZ)));
 }
 return nullptr;
}
Chunk& ChunkCache::adoptChunk(int chunkX, int chunkZ, std::unique_ptr<Chunk> owned) {
 const ChunkPos pos{chunkX, chunkZ};
 Chunk* chunk = nullptr;
 if(owned != nullptr) {
  owned->world = world_;
  if(world_ != nullptr) {
   owned->lastSaveTime = static_cast<long long>(world_->getTime());
  }
  chunk = owned.get();
  chunk->dataReady = true;
  ownedChunks_.emplace(chunk, std::move(owned));
 } else {
  chunk = &empty_;
 }
 chunksByPos_[pos] = chunk;
 chunks_.push_back(chunk);
 if(chunk != &empty_) {
  chunk->populateBlockLight();
  chunk->load();
 }
 if(generator_ != nullptr) {
  if(chunk != nullptr && !chunk->terrainPopulated && isChunkLoaded(chunkX + 1, chunkZ + 1) &&
     isChunkLoaded(chunkX, chunkZ + 1) && isChunkLoaded(chunkX + 1, chunkZ)) {
   decorate(this, chunkX, chunkZ);
  }
  if(isChunkLoaded(chunkX - 1, chunkZ) && !getChunk(chunkX - 1, chunkZ).terrainPopulated &&
     isChunkLoaded(chunkX - 1, chunkZ + 1) && isChunkLoaded(chunkX, chunkZ + 1) &&
     isChunkLoaded(chunkX - 1, chunkZ)) {
   decorate(this, chunkX - 1, chunkZ);
  }
  if(isChunkLoaded(chunkX, chunkZ - 1) && !getChunk(chunkX, chunkZ - 1).terrainPopulated &&
     isChunkLoaded(chunkX + 1, chunkZ - 1) && isChunkLoaded(chunkX, chunkZ - 1) &&
     isChunkLoaded(chunkX + 1, chunkZ)) {
   decorate(this, chunkX, chunkZ - 1);
  }
  if(isChunkLoaded(chunkX - 1, chunkZ - 1) && !getChunk(chunkX - 1, chunkZ - 1).terrainPopulated &&
     isChunkLoaded(chunkX - 1, chunkZ - 1) && isChunkLoaded(chunkX, chunkZ - 1) &&
     isChunkLoaded(chunkX - 1, chunkZ)) {
   decorate(this, chunkX - 1, chunkZ - 1);
  }
 }
 if(world_ != nullptr && chunk != &empty_) {
  // Neighboring sections may have meshed against this then-missing chunk
  // (treated as sky-lit air), leaving stale water faces and light seams at
  // the border. Dirty a one-block shell around this chunk so those border
  // sections re-mesh now that real blocks and light exist.
  const int blockX = chunkX * 16;
  const int blockZ = chunkZ * 16;
  world_->setBlocksDirty(blockX - 1, 0, blockZ - 1, blockX + 16, Chunk::height - 1, blockZ + 16);
 }
 return *chunk;
}
Chunk& ChunkCache::getChunk(int chunkX, int chunkZ) {
 const auto it = chunksByPos_.find(ChunkPos{chunkX, chunkZ});
 if(it == chunksByPos_.end()) {
  if(world_ != nullptr && (world_->isEventProcessingEnabled() || forceLoad)) {
   return loadChunk(chunkX, chunkZ);
  }
  return empty_;
 }
 return *it->second;
}
void ChunkCache::requestChunkAsync(int chunkX, int chunkZ) {
 if(world_ == nullptr || (storage_ == nullptr && generator_ == nullptr)) {
  return;
 }
 const ChunkPos pos{chunkX, chunkZ};
 if(chunksByPos_.contains(pos) || pendingLoads_.contains(pos)) {
  return;
 }
 auto pending = std::make_shared<PendingLoad>();
 pending->chunkX = chunkX;
 pending->chunkZ = chunkZ;
 pendingLoads_.emplace(pos, pending);
 if(loaderPool_ == nullptr) {
  loaderPool_ = std::make_unique<net::minecraft::util::concurrent::WorkerPool>(1U);
 }
 loaderPool_->submit([this, pending] {
  try {
   pending->chunk = produceChunk(pending->chunkX, pending->chunkZ);
  } catch(...) {
   pending->chunk.reset();
  }
  pending->done.store(true, std::memory_order_release);
 });
}
void ChunkCache::integrateFinishedLoads(int budget) {
 while(budget > 0) {
  auto best = pendingLoads_.end();
  int bestDistance = std::numeric_limits<int>::max();
  for(auto it = pendingLoads_.begin(); it != pendingLoads_.end(); ++it) {
   const PendingLoad& pending = *it->second;
   if(!pending.done.load(std::memory_order_acquire)) {
    continue;
   }
   const int distance =
       std::max(std::abs(pending.chunkX - centerChunkX_), std::abs(pending.chunkZ - centerChunkZ_));
   if(distance > activeRadius_) {
    best = it;
    bestDistance = distance;
    break;
   }
   if(distance < bestDistance) {
    best = it;
    bestDistance = distance;
   }
  }
  if(best == pendingLoads_.end()) {
   break;
  }
  PendingLoad& pending = *best->second;
  std::unique_ptr<Chunk> chunk = std::move(pending.chunk);
  const int chunkX = pending.chunkX;
  const int chunkZ = pending.chunkZ;
  pendingLoads_.erase(best);
  const ChunkPos pos{chunkX, chunkZ};
  if(chunk == nullptr || chunksByPos_.contains(pos) ||
     std::max(std::abs(chunkX - centerChunkX_), std::abs(chunkZ - centerChunkZ_)) > activeRadius_) {
   continue;
  }
  chunksToUnload_.erase(pos);
  adoptChunk(chunkX, chunkZ, std::move(chunk));
  --budget;
 }
}
void ChunkCache::saveEntities(Chunk& chunk) {
 if(storage_ == nullptr || world_ == nullptr) {
  return;
 }
 const std::lock_guard lock(ioMutex_);
 try {
  storage_->saveEntities(world_, chunk);
 } catch(...) {
 }
}
void ChunkCache::saveChunk(Chunk& chunk) {
 if(storage_ == nullptr || world_ == nullptr) {
  return;
 }
 const std::lock_guard lock(ioMutex_);
 try {
  chunk.lastSaveTime = static_cast<long long>(world_->getTime());
  storage_->saveChunk(world_, chunk);
 } catch(...) {
 }
}
void ChunkCache::decorate(ChunkSource* source, int chunkX, int chunkZ) {
 Chunk& chunk = getChunk(chunkX, chunkZ);
 if(chunk.terrainPopulated) {
  return;
 }
 chunk.terrainPopulated = true;
 if(generator_ != nullptr) {
  const std::lock_guard lock(ioMutex_);
  generator_->decorate(source, chunkX, chunkZ);
  chunk.markDirty();
 }
}
bool ChunkCache::save(bool saveEntityData, client::gui::screen::LoadingDisplay* display) {
 (void)display;
 int saved = 0;
 for(Chunk* chunk : chunks_) {
  if(chunk == nullptr) {
   continue;
  }
  if(saveEntityData && !chunk->empty) {
   saveEntities(*chunk);
  }
  if(!chunk->shouldSave(saveEntityData)) {
   continue;
  }
  saveChunk(*chunk);
  chunk->dirty = false;
  if(++saved == 24 && !saveEntityData) {
   if(storage_ != nullptr) {
    const std::lock_guard lock(ioMutex_);
    storage_->flush();
   }
   return false;
  }
 }
 if(storage_ != nullptr && (saveEntityData || saved > 0)) {
  const std::lock_guard lock(ioMutex_);
  storage_->flush();
 }
 return true;
}
bool ChunkCache::tick() {
 integrateFinishedLoads(8);
 if(world_ != nullptr && !world_->isSavingDisabled()) {
  for(int i = 0; i < 100; ++i) {
   if(chunksToUnload_.empty()) {
    break;
   }
   const ChunkPos pos = *chunksToUnload_.begin();
   chunksToUnload_.erase(chunksToUnload_.begin());
   const auto mapIt = chunksByPos_.find(pos);
   if(mapIt == chunksByPos_.end()) {
    continue;
   }
   Chunk* chunk = mapIt->second;
   retireFromLighting(chunk);
   chunk->unload();
   if(chunk->shouldSave(true)) {
    saveChunk(*chunk);
    saveEntities(*chunk);
    chunk->dirty = false;
   }
   chunksByPos_.erase(mapIt);
   chunks_.erase(std::remove(chunks_.begin(), chunks_.end(), chunk), chunks_.end());
   ownedChunks_.erase(chunk);
  }
  if(storage_ != nullptr) {
   const std::lock_guard lock(ioMutex_);
   storage_->tick();
  }
 }
 if(generator_ == nullptr) {
  return false;
 }
 const std::lock_guard lock(ioMutex_);
 return generator_->tick();
}
bool ChunkCache::canSave() const {
 return world_ == nullptr || !world_->isSavingDisabled();
}
void ChunkCache::setActiveRadius(int radius) {
 activeRadius_ = std::max(0, radius);
}
void ChunkCache::setChunkCacheCenter(int chunkX, int chunkZ) {
 centerChunkX_ = chunkX;
 centerChunkZ_ = chunkZ;
}
void ChunkCache::pumpChunkPublish() {
 integrateFinishedLoads(32);
}
void ChunkCache::prefetchChunksNear(int centerChunkX, int centerChunkZ) {
 setChunkCacheCenter(centerChunkX, centerChunkZ);
 const bool asyncCapable = world_ != nullptr && (storage_ != nullptr || generator_ != nullptr);
 if(!asyncCapable) {
  return;
 }
 const int maxLoadsPerCall = 16;
 int loaded = 0;
 integrateFinishedLoads(4);
 for(int ring = 0; ring <= activeRadius_ && loaded < maxLoadsPerCall; ++ring) {
  for(int dx = -ring; dx <= ring && loaded < maxLoadsPerCall; ++dx) {
   for(int dz = -ring; dz <= ring && loaded < maxLoadsPerCall; ++dz) {
    if(std::max(std::abs(dx), std::abs(dz)) != ring) {
     continue;
    }
    const int cx = centerChunkX + dx;
    const int cz = centerChunkZ + dz;
    if(isChunkLoaded(cx, cz)) {
     continue;
    }
    if(ring > 1) {
     if(pendingLoads_.contains(ChunkPos{cx, cz})) {
      continue;
     }
     requestChunkAsync(cx, cz);
    } else {
     // Innermost rings load synchronously: player movement is gated on the
     // surrounding region being resident, so these can never lag behind.
     loadChunk(cx, cz);
    }
    ++loaded;
   }
  }
 }
 for(const auto& [pos, chunk] : chunksByPos_) {
  (void)chunk;
  if(std::max(std::abs(pos.x - centerChunkX), std::abs(pos.z - centerChunkZ)) > activeRadius_) {
   chunksToUnload_.insert(pos);
  }
 }
}
std::string ChunkCache::getDebugInfo() const {
 return "ChunkCache: " + std::to_string(chunksByPos_.size()) + " Drop: " + std::to_string(chunksToUnload_.size());
}
} // namespace net::minecraft::world::chunk
