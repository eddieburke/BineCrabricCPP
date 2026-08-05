#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <thread>
#include "net/minecraft/util/concurrent/FrameBudget.hpp"
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
bool ChunkCache::retireFromLighting(Chunk* chunk) {
 if(chunk == nullptr || chunk->isEmpty()) {
  return true;
 }
 // Bounded wait for in-flight render pins (mesh/lighting jobs) to drain instead
 // of an unbounded spin. A chunk whose arrays a worker is still copying cannot be
 // unloaded safely; but if the pins do not drain quickly (e.g. workers blocked
 // behind a full completed-job channel the main thread would have to drain), the
 // spin would hang the caller forever. On timeout, cancel the eviction marker so
 // the chunk stays usable and the caller retries the eviction later.
 constexpr auto kEvictionTimeout = std::chrono::milliseconds(10);
 const auto deadline = std::chrono::steady_clock::now() + kEvictionTimeout;
 while(!chunk->beginRenderEviction()) {
  if(std::chrono::steady_clock::now() >= deadline) {
   chunk->cancelRenderEviction();
   return false;
  }
  std::this_thread::sleep_for(std::chrono::microseconds(200));
 }
 return true;
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
  if(!retireFromLighting(chunk)) {
   // Still pinned by an in-flight mesh/lighting job; keep the chunk loaded and
   // let a later drain retry the eviction.
   return;
  }
  chunk->unload();
 }
 chunks_.erase(std::remove(chunks_.begin(), chunks_.end(), chunk), chunks_.end());
 chunksByPos_.erase(it);
 ownedChunks_.erase(chunk);
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
    const std::lock_guard lock(storageMutex_);
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
  {
   const ChunkRenderWriteScope guard(*chunk);
   chunk->populateBlockLight();
  }
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
     net::minecraft::util::concurrent::Domain::Compute).submit(
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
 // Single O(n) sweep instead of one full scan per adopted chunk: drop finished
 // dead (cancelled) and out-of-radius entries, and collect the ready in-radius
 // candidates ordered by distance. Adoption then walks the collected list with
 // the per-call budget and wall-clock budget.
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
  const int distance =
      std::max(std::abs(pending->chunkX - centerChunkX_), std::abs(pending->chunkZ - centerChunkZ_));
  if(distance > activeRadius_) {
   it = pendingLoads_.erase(it);
   continue;
  }
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
 const std::lock_guard lock(storageMutex_);
 try {
  storage_->saveEntities(world_, chunk);
 } catch(...) {
 }
}
void ChunkCache::enqueueSerializedWrite(int chunkX, int chunkZ, std::vector<std::uint8_t> raw) {
 if(storage_ == nullptr) {
  return;
 }
 pendingSaveWrites_.fetch_add(1, std::memory_order_acq_rel);
 bool schedule = false;
 {
  const std::lock_guard lock(saveQueueMutex_);
  saveQueue_.push_back(SerializedWrite{chunkX, chunkZ, std::move(raw), nullptr});
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
void ChunkCache::enqueueSerializedWrite(int chunkX, int chunkZ, AlphaChunkStorage::ChunkSnapshot snapshot) {
 if(storage_ == nullptr) {
  return;
 }
 pendingSaveWrites_.fetch_add(1, std::memory_order_acq_rel);
 bool schedule = false;
 {
  const std::lock_guard lock(saveQueueMutex_);
  saveQueue_.push_back(
      SerializedWrite{chunkX, chunkZ, {}, std::make_unique<AlphaChunkStorage::ChunkSnapshot>(std::move(snapshot))});
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
   }
   // Unlocked: every ChunkStorage implementation serializes its own disk access
   // (RegionIo holds a per-region lock, AlphaChunkStorage a file lock). Holding
   // storageMutex_ across the compress-and-write here parked the main thread's
   // tick() and every compute worker's produceChunk() behind disk I/O.
   storage_->writeSerializedChunk(write.chunkX, write.chunkZ, write.raw);
  } catch(...) {
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
  const ChunkRenderWriteScope guard(chunk);
  if(storage_->supportsAsyncWrites()) {
   world_->checkSessionLock();
   AlphaChunkStorage::ChunkSnapshot snapshot = AlphaChunkStorage::takeSnapshot(chunk, world_);
   enqueueSerializedWrite(chunk.x, chunk.z, std::move(snapshot));
  } else {
   const std::lock_guard lock(storageMutex_);
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
  // No storage lock here. Decoration is main-thread-only (adoptChunk is the sole
  // caller) and touches only main-thread state, so it needs no mutual exclusion
  // of its own. Taking storageMutex_ across it used to stall every compute worker
  // in produceChunk for the whole decoration pass -- and adoptChunk can decorate
  // up to four columns per adopted chunk, up to 32 chunks per publish, so terrain
  // generation was effectively serialized behind main-thread decoration.
  const ChunkRenderWriteScope guard(chunk);
  generator_->decorate(source, chunkX, chunkZ);
  chunk.markDirty();
 }
}
bool ChunkCache::save(bool saveEntityData, client::gui::screen::LoadingDisplay* display) {
 (void)display;
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
   const std::lock_guard lock(storageMutex_);
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
  if(!retireFromLighting(chunk)) {
   // Pinned by an in-flight mesh/lighting job; leave the chunk loaded and retry
   // the eviction on a later drain instead of blocking this frame.
   chunksToUnload_.insert(pos);
   continue;
  }
  chunk->unload();
  if(chunk->shouldSave(true)) {
   saveChunk(*chunk);
   saveEntities(*chunk);
   chunk->dirty = false;
  }
  chunksByPos_.erase(mapIt);
  chunks_.erase(std::remove(chunks_.begin(), chunks_.end(), chunk), chunks_.end());
  ownedChunks_.erase(chunk);
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
   const std::lock_guard lock(storageMutex_);
   storage_->tick();
  }
 }
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
 activeRadius_ = std::max(0, radius);
}
void ChunkCache::setChunkCacheCenter(int chunkX, int chunkZ) {
 centerChunkX_ = chunkX;
 centerChunkZ_ = chunkZ;
 for(auto it = pendingLoads_.begin(); it != pendingLoads_.end();) {
  const ChunkPos position = it->first;
  if(std::max(std::abs(position.x - centerChunkX_), std::abs(position.z - centerChunkZ_)) <= activeRadius_) {
   ++it;
   continue;
  }
  it->second->cancelledGeneration.store(it->second->generation, std::memory_order_release);
  it = pendingLoads_.erase(it);
 }
 for(const auto& [pos, chunk] : chunksByPos_) {
  (void)chunk;
  if(std::max(std::abs(pos.x - centerChunkX_), std::abs(pos.z - centerChunkZ_)) > activeRadius_) {
   chunksToUnload_.insert(pos);
  }
 }
}
void ChunkCache::pumpChunkPublish() {
 // Pacing: each adopted chunk can run main-thread decoration + block-light
 // population (tens of ms per chunk in debug builds). Cap this render-frame
 // publish to a few ms so a backlog of ready chunks cannot turn one frame into
 // a multi-second hitch. The count cap still bounds the burst when adoption is
 // cheap, and tick() keeps streaming server-side chunks at its own rate.
  const auto& frame = net::minecraft::util::concurrent::FrameBudget::frameDeadline();
  std::int64_t budget = 16'000'000;
  if(frame.active()) {
   if(frame.expired()) {
    // The shared frame deadline is already in the past (this frame ran over
    // budget). Clamping to 0 here would throttle adoption to exactly one chunk
    // per frame while prefetchChunksNear keeps queueing up to 16/tick, so the
    // world stream crawls and the renderer's section counters climb forever.
    // Fall back to a fresh local slice so the backlog keeps draining at a sane
    // rate during overloaded frames.
    budget = 4'000'000;
   } else {
    budget = frame.remaining().count();
   }
  }
  integrateFinishedLoads(32, budget);
 if(world_ != nullptr && !world_->isSavingDisabled()) {
  drainChunksToUnload(100);
 }
}
void ChunkCache::prefetchChunksNear(int centerChunkX, int centerChunkZ) {
 setChunkCacheCenter(centerChunkX, centerChunkZ);
 for(const auto& [pos, chunk] : chunksByPos_) {
  (void)chunk;
  if(std::max(std::abs(pos.x - centerChunkX), std::abs(pos.z - centerChunkZ)) > activeRadius_) {
   chunksToUnload_.insert(pos);
  }
 }
 if(world_ == nullptr || (storage_ == nullptr && generator_ == nullptr)) {
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
    if(pendingLoads_.contains(ChunkPos{cx, cz})) {
     continue;
    }
    const int priority = ring <= 1 ? std::numeric_limits<int>::min() : ring;
    requestChunkAsync(cx, cz, priority);
    ++loaded;
   }
  }
 }
}
std::string ChunkCache::getDebugInfo() const {
 return "Chunks loaded: " + std::to_string(chunksByPos_.size()) +
        ", Pending unload: " + std::to_string(chunksToUnload_.size());
}
} // namespace net::minecraft::world::chunk
