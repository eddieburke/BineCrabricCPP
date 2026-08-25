#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <thread>
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace net::minecraft::world::chunk {
namespace {
void setIoThreadPriority() {
#ifdef _WIN32
 SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
}
} // namespace
ChunkCache::ChunkCache(World* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator)
    : empty_(world, 0, 0), world_(world), storage_(std::move(storage)), generator_(generator) {
}
ChunkCache::~ChunkCache() {
 for(auto& [position, pending] : pendingLoads_) {
  (void)position;
  pending->cancelledGeneration.store(pending->generation, std::memory_order_release);
 }
 waitForPendingLoads();
 waitForPendingWrites();
 {
  const std::lock_guard lock(workerGeneratorMutex_);
  workerGenerators_.clear();
 }
 for(const auto& entry : graveyard_) {
  if(entry != nullptr && entry->renderPinCount() != 0) {
   Chunk::waitForPinDrain(*entry, std::chrono::milliseconds(5000));
  }
 }
}
bool ChunkCache::isChunkLoaded(int chunkX, int chunkZ) const {
 return chunksByPos_.find(ChunkPos{chunkX, chunkZ}) != chunksByPos_.end();
}
Chunk* ChunkCache::getChunkIfLoaded(int chunkX, int chunkZ) {
 const auto it = chunksByPos_.find(ChunkPos{chunkX, chunkZ});
 return it == chunksByPos_.end() ? nullptr : it->second;
}
bool ChunkCache::isChunkDataReady(int chunkX, int chunkZ) const {
 const auto it = chunksByPos_.find(ChunkPos{chunkX, chunkZ});
 return it != chunksByPos_.end() && it->second != nullptr && it->second->dataReady;
}
void ChunkCache::markChunkDataReady(int chunkX, int chunkZ) {
 const auto it = chunksByPos_.find(ChunkPos{chunkX, chunkZ});
 if(it != chunksByPos_.end() && it->second != nullptr) {
  const bool becameReady = !it->second->dataReady;
  it->second->dataReady = true;
  if(becameReady && world_ != nullptr && it->second != &empty_) {
   world_->chunkAvailable(chunkX, chunkZ);
  }
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
void ChunkCache::unloadChunk(int chunkX, int chunkZ) {
 const ChunkPos pos{chunkX, chunkZ};
 if(const auto pending = pendingLoads_.find(pos); pending != pendingLoads_.end()) {
  pending->second->cancelledGeneration.store(pending->second->generation, std::memory_order_release);
  pendingLoads_.erase(pending);
 }
 auto it = chunksByPos_.find(pos);
 if(it == chunksByPos_.end()) {
  return;
 }
 Chunk* chunk = it->second;
 if(chunk != nullptr && !chunk->isEmpty()) {
  chunk->markEvicted();
  chunk->unload();
 }
 chunks_.erase(std::remove(chunks_.begin(), chunks_.end(), chunk), chunks_.end());
 chunksByPos_.erase(it);
 const auto ownedIt = ownedChunks_.find(chunk);
 if(ownedIt != ownedChunks_.end()) {
  // Tombstone: keep the object alive until worker leases drain, then sweep.
  graveyard_.push_back(std::move(ownedIt->second));
  ownedChunks_.erase(ownedIt);
 }
 if(world_ != nullptr) {
  world_->chunkUnloaded(chunkX, chunkZ);
 }
}
ChunkSource* ChunkCache::workerGenerator() {
 if(generator_ == nullptr || world_ == nullptr || world_->dimension == nullptr) {
  return generator_;
 }
 const std::thread::id threadId = std::this_thread::get_id();
 {
  const std::lock_guard lock(workerGeneratorMutex_);
  const auto it = workerGenerators_.find(threadId);
  if(it != workerGenerators_.end()) {
   return it->second.get();
  }
 }
 std::unique_ptr<ChunkSource> clone = world_->dimension->createChunkGeneratorFromSeed(world_->getSeed());
 if(clone == nullptr) {
  return generator_;
 }
 ChunkSource* raw = clone.get();
 const std::lock_guard lock(workerGeneratorMutex_);
 workerGenerators_.emplace(threadId, std::move(clone));
 return raw;
}
std::unique_ptr<Chunk> ChunkCache::produceChunk(int chunkX, int chunkZ) {
 if(storage_ != nullptr && world_ != nullptr) {
  try {
   std::unique_ptr<Chunk> loaded;
   {
    loaded = std::make_unique<Chunk>(std::move(storage_->loadChunk(world_, chunkX, chunkZ)));
   }
   if(!loaded->empty && loaded->chunkPosEquals(chunkX, chunkZ)) {
    return loaded;
   }
  } catch(...) {
  }
 }
 ChunkSource* generator = workerGenerator();
 if(generator != nullptr) {
  return std::make_unique<Chunk>(std::move(generator->getChunk(chunkX, chunkZ)));
 }
 return nullptr;
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
  const std::shared_ptr<PendingLoad> pending = pendingIt->second;
  pendingLoads_.erase(pendingIt);
  if(pending->done.load(std::memory_order_acquire) && pending->chunk != nullptr) {
   return adoptChunk(chunkX, chunkZ, std::move(pending->chunk));
  }
  pending->cancelledGeneration.store(pending->generation, std::memory_order_release);
 }
 if(generator_ == nullptr && storage_ == nullptr) {
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
  world_->chunkAvailable(chunkX, chunkZ);
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
void ChunkCache::requestChunkAsync(int chunkX, int chunkZ, int priority) {
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
 pending->generation = nextLoadGeneration_++;
 pendingLoads_.emplace(pos, pending);
 pendingLoadTasks_.fetch_add(1, std::memory_order_acq_rel);
 net::minecraft::util::concurrent::ThreadCoordinator::instance().pool(
                                                                    net::minecraft::util::concurrent::Domain::Compute)
     .submit(
         [this, pending] {
          if(pending->cancelledGeneration.load(std::memory_order_acquire) != pending->generation) {
           try {
            pending->chunk = produceChunk(pending->chunkX, pending->chunkZ);
           } catch(...) {
            pending->chunk.reset();
           }
          }
          pending->done.store(true, std::memory_order_release);
          if(pendingLoadTasks_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
           const std::lock_guard lock(loadCompleteMutex_);
           loadCompleteCv_.notify_all();
          }
         },
         priority);
}
void ChunkCache::integrateFinishedLoads(int budget, std::int64_t timeBudgetNs) {
 const auto start = std::chrono::steady_clock::now();
 struct Candidate {
  std::shared_ptr<PendingLoad> load;
  int distance = 0;
 };
 std::vector<Candidate> ready;
 for(auto it = pendingLoads_.begin(); it != pendingLoads_.end();) {
  const std::shared_ptr<PendingLoad>& pending = it->second;
  if(!pending->done.load(std::memory_order_acquire)) {
   ++it;
   continue;
  }
  if(pending->cancelledGeneration.load(std::memory_order_acquire) == pending->generation) {
   it = pendingLoads_.erase(it);
   continue;
  }
  const ChunkPos position{pending->chunkX, pending->chunkZ};
  if(!isDesired(position)) {
   it = pendingLoads_.erase(it);
   continue;
  }
  const int dx = pending->chunkX - centerChunkX_;
  const int dz = pending->chunkZ - centerChunkZ_;
  const int distance = dx * dx + dz * dz;
  ready.push_back({pending, distance});
  ++it;
 }
 std::stable_sort(ready.begin(), ready.end(),
                  [](const Candidate& a, const Candidate& b) { return a.distance < b.distance; });
 int adopted = 0;
 for(Candidate& candidate : ready) {
  if(budget <= 0) break;
  if(adopted > 0 && timeBudgetNs >= 0 &&
     std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count() >=
         timeBudgetNs) {
   break;
  }
  PendingLoad& pending = *candidate.load;
  const int chunkX = pending.chunkX;
  const int chunkZ = pending.chunkZ;
  const ChunkPos pos{chunkX, chunkZ};
  if(pending.chunk == nullptr || chunksByPos_.contains(pos)) {
   pendingLoads_.erase(pos);
   if(pending.chunk == nullptr && isDesired(pos)) {
    residencyQueue_.push_back(pos);
   }
   continue;
  }
  pendingLoads_.erase(pos);
  chunksToUnload_.erase(pos);
  adoptChunk(chunkX, chunkZ, std::move(pending.chunk));
  ++adopted;
  --budget;
 }
}
void ChunkCache::saveEntities(Chunk& chunk) {
 if(storage_ == nullptr || world_ == nullptr) {
  return;
 }
 try {
  storage_->saveEntities(world_, chunk);
 } catch(...) {
 }
}
void ChunkCache::enqueueSnapshotWrite(Chunk* chunk, std::uint64_t saveTime) {
 if(storage_ == nullptr || chunk == nullptr) {
  return;
 }
 pendingSaveWrites_.fetch_add(1, std::memory_order_acq_rel);
 bool schedule = false;
 {
  const std::lock_guard lock(saveQueueMutex_);
  saveQueue_.push_back(SerializedWrite{chunk->x, chunk->z, {}, nullptr, chunk, saveTime});
  if(!saveDrainScheduled_) {
   saveDrainScheduled_ = true;
   schedule = true;
  }
 }
 if(schedule) {
  net::minecraft::util::concurrent::ThreadCoordinator::instance()
      .pool(net::minecraft::util::concurrent::Domain::Io)
      .submit([this] { drainSerializedWrites(); });
 }
}
void ChunkCache::drainSerializedWrites() {
 setIoThreadPriority();
 for(;;) {
  SerializedWrite write;
  {
   const std::lock_guard lock(saveQueueMutex_);
   if(saveQueue_.empty()) {
    saveDrainScheduled_ = false;
    return;
   }
   write = std::move(saveQueue_.front());
   saveQueue_.pop_front();
  }
  try {
   if(write.snapshot != nullptr) {
    AlphaChunkStorage::writeRootChunkFromSnapshot(write.raw, *write.snapshot);
   } else if(write.chunk != nullptr) {
    // The render lease keeps the chunk alive for the whole snapshot even if
    // the main thread evicts it concurrently; release it once serialized.
    const AlphaChunkStorage::ChunkSnapshot snapshot =
        AlphaChunkStorage::takeSnapshot(*write.chunk, write.saveTime);
    write.chunk->lastSaveHadEntities.store(
        !snapshot.entities.storage().asList().empty(), std::memory_order_relaxed);
    write.chunk->releaseRenderPin();
    write.chunk = nullptr;
    AlphaChunkStorage::writeRootChunkFromSnapshot(write.raw, snapshot);
   }
   // Unlocked: every ChunkStorage implementation serializes its own disk access
   // (RegionIo holds a per-region lock, AlphaChunkStorage a per-file lock).
   storage_->writeSerializedChunk(write.chunkX, write.chunkZ, write.raw);
  } catch(...) {
   if(write.chunk != nullptr) {
    write.chunk->releaseRenderPin();
   }
  }
  completeSerializedWrite();
 }
}
void ChunkCache::completeSerializedWrite() {
 if(pendingSaveWrites_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
  const std::lock_guard lock(saveCompleteMutex_);
  saveCompleteCv_.notify_all();
 }
}
void ChunkCache::waitForPendingWrites() {
 std::unique_lock lock(saveCompleteMutex_);
 saveCompleteCv_.wait(lock, [this] { return pendingSaveWrites_.load(std::memory_order_acquire) == 0; });
}
void ChunkCache::waitForPendingLoads() {
 std::unique_lock lock(loadCompleteMutex_);
 loadCompleteCv_.wait(lock, [this] { return pendingLoadTasks_.load(std::memory_order_acquire) == 0; });
}
void ChunkCache::saveChunk(Chunk& chunk) {
 if(storage_ == nullptr || world_ == nullptr) {
  return;
 }
 try {
  chunk.lastSaveTime = static_cast<long long>(world_->getTime());
  if(storage_->supportsAsyncWrites()) {
   // Same reason as AlphaChunkStorage::writeRootChunk: the array is already the
   // right size, and resizing a live one races the lock-free mesh capture.
   if(!chunk.tryAcquireRenderPin()) {
    // Chunk is already evicted; nothing to save (its data was saved before the
    // eviction tombstone was set).
    return;
   }
   enqueueSnapshotWrite(&chunk, chunk.lastSaveTime);
  } else {
   storage_->saveChunk(world_, chunk);
  }
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
  // Decoration is main-thread-only (adoptChunk is the sole caller) and touches
  // only main-thread state, so it needs no mutual exclusion of its own.
  generator_->decorate(source, chunkX, chunkZ);
  chunk.markDirty();
 }
}
bool ChunkCache::save(bool saveEntityData, SaveProgressCallback progress) {
 (void)progress;
 if(world_ != nullptr) {
  try {
   // One session-lock verification per save batch instead of per chunk: the
   // world-level save paths also verify, and per-chunk checks were one file
   // open+read on the main thread for every chunk saved.
   world_->checkSessionLock();
  } catch(...) {
  }
 }
 int saved = 0;
 constexpr int kAutosaveBudget = 8;
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
  ++saved;
  if(!saveEntityData && saved >= kAutosaveBudget) {
   pendingIncrementalSave_ = true;
   return false;
  }
 }
 pendingIncrementalSave_ = false;
 if(saveEntityData) {
  waitForPendingWrites();
 }
 if(storage_ != nullptr && (saveEntityData || saved > 0)) {
  if(saveEntityData) {
   storage_->flush();
  }
 }
 return true;
}
void ChunkCache::prepareForSave() {
 pendingIncrementalSave_ = false;
 waitForPendingWrites();
}
void ChunkCache::drainChunksToUnload(int maxChunks) {
 for(int i = 0; i < maxChunks; ++i) {
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
  if(chunk->shouldSave(true)) {
   saveChunk(*chunk);
   saveEntities(*chunk);
   chunk->dirty = false;
  }
  chunk->markEvicted();
  chunk->unload();
  chunksByPos_.erase(mapIt);
  chunks_.erase(std::remove(chunks_.begin(), chunks_.end(), chunk), chunks_.end());
  const auto ownedIt = ownedChunks_.find(chunk);
  if(ownedIt != ownedChunks_.end()) {
   graveyard_.push_back(std::move(ownedIt->second));
   ownedChunks_.erase(ownedIt);
  }
  if(world_ != nullptr) {
   world_->chunkUnloaded(pos.x, pos.z);
  }
 }
}
bool ChunkCache::tick() {
 integrateFinishedLoads(2);
 if(pendingIncrementalSave_ && world_ != nullptr && !world_->isSavingDisabled()) {
  save(false, nullptr);
 }
 if(world_ != nullptr && !world_->isSavingDisabled()) {
  drainChunksToUnload(100);
  if(storage_ != nullptr) {
   storage_->tick();
  }
 }
 sweepGraveyard();
 if(generator_ == nullptr) {
  return false;
 }
 // generator_ is the main-thread generator; workers run their own clones from
 // workerGenerators_, so this needs no storage lock.
 return generator_->tick();
}
bool ChunkCache::canSave() const {
 return world_ == nullptr || !world_->isSavingDisabled();
}
void ChunkCache::setActiveRadius(int radius) {
 const int resolved = std::max(0, radius);
 if(activeRadius_ == resolved) {
  return;
 }
 activeRadius_ = resolved;
 rebuildResidencyPlan();
}
void ChunkCache::setChunkCacheCenter(int chunkX, int chunkZ) {
 if(centerChunkX_ == chunkX && centerChunkZ_ == chunkZ && plannedRadius_ == activeRadius_) {
  return;
 }
 centerChunkX_ = chunkX;
 centerChunkZ_ = chunkZ;
 rebuildResidencyPlan();
}
bool ChunkCache::isDesired(const ChunkPos& position) const noexcept {
 if(plannedRadius_ == activeRadius_ && plannedCenterChunkX_ == centerChunkX_ &&
    plannedCenterChunkZ_ == centerChunkZ_) {
  return desiredChunks_.contains(position);
 }
 const std::int64_t dx = static_cast<std::int64_t>(position.x) - centerChunkX_;
 const std::int64_t dz = static_cast<std::int64_t>(position.z) - centerChunkZ_;
 const std::int64_t radius = activeRadius_;
 return dx * dx + dz * dz <= radius * radius;
}
void ChunkCache::rebuildResidencyPlan() {
 if(plannedCenterChunkX_ == centerChunkX_ && plannedCenterChunkZ_ == centerChunkZ_ &&
    plannedRadius_ == activeRadius_) {
  return;
 }
 plannedCenterChunkX_ = centerChunkX_;
 plannedCenterChunkZ_ = centerChunkZ_;
 plannedRadius_ = activeRadius_;
 struct Candidate {
  ChunkPos position{};
  std::int64_t distanceSq = 0;
 };
 std::vector<Candidate> order;
 const int diameter = activeRadius_ * 2 + 1;
 order.reserve(static_cast<std::size_t>(diameter * diameter));
 desiredChunks_.clear();
 residencyQueue_.clear();
 const std::int64_t radiusSq = static_cast<std::int64_t>(activeRadius_) * activeRadius_;
 for(int dx = -activeRadius_; dx <= activeRadius_; ++dx) {
  for(int dz = -activeRadius_; dz <= activeRadius_; ++dz) {
   const std::int64_t distanceSq = static_cast<std::int64_t>(dx) * dx + static_cast<std::int64_t>(dz) * dz;
   if(distanceSq > radiusSq) {
    continue;
   }
   order.push_back({ChunkPos{centerChunkX_ + dx, centerChunkZ_ + dz}, distanceSq});
  }
 }
 std::sort(order.begin(), order.end(), [](const Candidate& a, const Candidate& b) {
  if(a.distanceSq != b.distanceSq) {
   return a.distanceSq < b.distanceSq;
  }
  if(a.position.x != b.position.x) {
   return a.position.x < b.position.x;
  }
  return a.position.z < b.position.z;
 });
 for(const Candidate& candidate : order) {
  desiredChunks_.insert(candidate.position);
  if(!chunksByPos_.contains(candidate.position) && !pendingLoads_.contains(candidate.position)) {
   residencyQueue_.push_back(candidate.position);
  }
 }
 for(auto it = pendingLoads_.begin(); it != pendingLoads_.end();) {
  const ChunkPos position = it->first;
  if(desiredChunks_.contains(position)) {
   ++it;
   continue;
  }
  it->second->cancelledGeneration.store(it->second->generation, std::memory_order_release);
  it = pendingLoads_.erase(it);
 }
 for(const auto& [pos, chunk] : chunksByPos_) {
  (void)chunk;
  if(!desiredChunks_.contains(pos)) {
   chunksToUnload_.insert(pos);
  } else {
   chunksToUnload_.erase(pos);
  }
 }
}
void ChunkCache::sweepGraveyard() {
 auto it = graveyard_.begin();
 while(it != graveyard_.end()) {
  if(*it == nullptr || (*it)->renderPinCount() == 0) {
   // Leases drained (or never taken): no worker can acquire a new one after
   // the eviction tombstone was set, so destruction is safe.
   it = graveyard_.erase(it);
  } else {
   ++it;
  }
 }
}
void ChunkCache::pumpChunkPublish() {
 constexpr std::int64_t kPublishBudgetNs = 4'000'000;
 integrateFinishedLoads(256, kPublishBudgetNs);
 refillAsyncLoads();
 if(world_ != nullptr && !world_->isSavingDisabled()) {
  drainChunksToUnload(100);
 }
 sweepGraveyard();
}
void ChunkCache::prefetchChunksNear(int centerChunkX, int centerChunkZ) {
 setChunkCacheCenter(centerChunkX, centerChunkZ);
 if(world_ == nullptr || (storage_ == nullptr && generator_ == nullptr)) {
  return;
 }
 integrateFinishedLoads(4);
 refillAsyncLoads();
}
void ChunkCache::refillAsyncLoads() {
 if(world_ == nullptr || (storage_ == nullptr && generator_ == nullptr)) {
  return;
 }
 const unsigned workerCount =
     net::minecraft::util::concurrent::ThreadCoordinator::instance()
         .pool(net::minecraft::util::concurrent::Domain::Compute)
         .threadCount();
 const std::size_t targetPending = asyncLoadWindow(workerCount);
 if(pendingLoads_.size() >= targetPending) {
  return;
 }
 const std::size_t maxLoads = targetPending - pendingLoads_.size();
 std::size_t loaded = 0;
 while(!residencyQueue_.empty() && loaded < maxLoads) {
  const ChunkPos position = residencyQueue_.front();
  residencyQueue_.pop_front();
  if(!isDesired(position) || chunksByPos_.contains(position) || pendingLoads_.contains(position)) {
   continue;
  }
  const int dx = position.x - centerChunkX_;
  const int dz = position.z - centerChunkZ_;
  const int distanceSq = dx * dx + dz * dz;
  const int priority = distanceSq <= 2 ? std::numeric_limits<int>::min() : distanceSq;
  requestChunkAsync(position.x, position.z, priority);
  ++loaded;
 }
}
void ChunkCache::forEachLoadedChunk(const LoadedChunkVisitor& visitor) {
 if(!visitor) {
  return;
 }
 for(const auto& [position, chunk] : chunksByPos_) {
  if(chunk != nullptr && chunk != &empty_ && chunk->dataReady) {
   visitor(position.x, position.z, *chunk);
  }
 }
}
std::string ChunkCache::getDebugInfo() const {
 return "Chunks loaded: " + std::to_string(chunksByPos_.size()) +
        ", Pending load: " + std::to_string(pendingLoads_.size()) +
        ", Queued: " + std::to_string(residencyQueue_.size()) +
        ", Pending unload: " + std::to_string(chunksToUnload_.size());
}
} // namespace net::minecraft::world::chunk
