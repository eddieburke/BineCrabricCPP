#include "net/minecraft/world/light/LightingEngine.hpp"
#include <algorithm>
#include <utility>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft {
bool LightingEngine::Box::expand(int x0, int y0, int z0, int x1, int y1, int z1) {
 if(x0 >= minX && y0 >= minY && z0 >= minZ && x1 <= maxX && y1 <= maxY && z1 <= maxZ) {
  return true;
 }
 constexpr int margin = 1;
 if(x0 < minX - margin || y0 < minY - margin || z0 < minZ - margin || x1 > maxX + margin || y1 > maxY + margin ||
    z1 > maxZ + margin) {
  return false;
 }
 const int nx0 = std::min(minX, x0);
 const int ny0 = std::min(minY, y0);
 const int nz0 = std::min(minZ, z0);
 const int nx1 = std::max(maxX, x1);
 const int ny1 = std::max(maxY, y1);
 const int nz1 = std::max(maxZ, z1);
 const int oldVol = (maxX - minX) * (maxY - minY) * (maxZ - minZ);
 const int newVol = (nx1 - nx0) * (ny1 - ny0) * (nz1 - nz0);
 if(newVol - oldVol > 2) {
  return false;
 }
 minX = nx0;
 minY = ny0;
 minZ = nz0;
 maxX = nx1;
 maxY = ny1;
 maxZ = nz1;
 return true;
}
void LightingEngine::Box::cover(int x0, int y0, int z0, int x1, int y1, int z1) {
 minX = std::min(minX, x0);
 minY = std::min(minY, y0);
 minZ = std::min(minZ, z0);
 maxX = std::max(maxX, x1);
 maxY = std::max(maxY, y1);
 maxZ = std::max(maxZ, z1);
}
LightingEngine::LightingEngine(world::light::UnifiedLightRegistry& registry)
    : lightRegistry_(registry),
      computePool_(util::concurrent::ThreadCoordinator::instance().pool(util::concurrent::Domain::Compute)),
      workerLimit_(util::concurrent::ThreadCoordinator::instance().computeShare(3)) {
}
void LightingEngine::push(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool merge) {
 if(stopping_.load(std::memory_order_relaxed)) {
  return;
 }
 {
  const std::lock_guard lock(stagingMutex_);
  if(merge) {
   const std::size_t n = std::min(staging_.size(), kStagingMergeScan);
   for(std::size_t i = 0; i < n; ++i) {
    Box& existing = staging_[staging_.size() - i - 1];
    if(existing.type == type && existing.expand(minX, minY, minZ, maxX, maxY, maxZ)) {
     return;
    }
   }
  }
  staging_.push_back(Box{type, minX, minY, minZ, maxX, maxY, maxZ});
  stagedCount_.store(staging_.size(), std::memory_order_relaxed);
  if(staging_.size() < kStagingFlushBoxes) {
   return;
  }
 }
 flushStaging();
}
void LightingEngine::flushStaging() {
 std::deque<Box> batch;
 {
  const std::lock_guard lock(stagingMutex_);
  if(staging_.empty()) {
   return;
  }
  batch.swap(staging_);
  stagedCount_.store(0, std::memory_order_relaxed);
 }
 const std::lock_guard lock(queueMutex_);
 if(stopping_.load(std::memory_order_relaxed)) {
  return;
 }
 constexpr std::size_t kMaxQueue = 200000;
 for(const Box& box : batch) {
  if(queue_.size() >= kMaxQueue) {
   const auto it = std::find_if(queue_.rbegin(), queue_.rend(), [&box](const Box& queued) {
    return queued.type == box.type;
   });
   if(it != queue_.rend()) {
    it->cover(box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
    continue;
   }
  }
  const std::int64_t volume = static_cast<std::int64_t>(box.maxX - box.minX + 1) *
                              static_cast<std::int64_t>(box.maxY - box.minY + 1) *
                              static_cast<std::int64_t>(box.maxZ - box.minZ + 1);
  if(volume <= 64) {
   queue_.push_front(box);
  } else {
   queue_.push_back(box);
  }
 }
 pendingCount_.store(queue_.size() + activeBoxes_.size(), std::memory_order_relaxed);
 scheduleWorkersLocked();
}
void LightingEngine::registerChunk(Chunk* chunk) {
 if(chunk == nullptr || chunk->isEmpty()) {
  return;
 }
 {
  const std::lock_guard lock(registryMutex_);
  registry_[chunkKey(chunk->x, chunk->z)] = chunk;
 }
}
void LightingEngine::unregisterChunk(Chunk* chunk) {
 if(chunk == nullptr) {
  return;
 }
 {
  const std::lock_guard lock(registryMutex_);
  const auto it = registry_.find(chunkKey(chunk->x, chunk->z));
  if(it != registry_.end() && it->second == chunk) {
   registry_.erase(it);
  }
 }
}
std::vector<LightingEngine::DirtyRegion> LightingEngine::drainDirtyRegions(std::size_t maxRegions) {
 flushStaging();
 flushPending(!busy());
 std::vector<DirtyRegion> regions;
 regions.reserve(std::min(maxRegions, outbox_.size()));
 DirtyRegion region{};
 while(regions.size() < maxRegions && outbox_.tryPop(region)) {
  regions.push_back(region);
 }
 return regions;
}
bool LightingEngine::hasDirtyRegions() const {
 if(outbox_.size() != 0) {
  return true;
 }
 const std::lock_guard lock(pendingMutex_);
 return !pending_.empty();
}
LightingEngine::TraceStats LightingEngine::traceStats() const {
 TraceStats stats;
 stats.stagedWork = stagedCount_.load(std::memory_order_relaxed);
 stats.pendingWork = pendingCount_.load(std::memory_order_relaxed);
 stats.publishedRegions = outbox_.size();
 const std::lock_guard lock(pendingMutex_);
 stats.pendingRegions = pending_.size();
 return stats;
}
void LightingEngine::stop() {
 {
  const std::lock_guard stagingLock(stagingMutex_);
  staging_.clear();
  stagedCount_.store(0, std::memory_order_relaxed);
 }
 {
  const std::lock_guard pendingLock(pendingMutex_);
  pending_.clear();
 }
 std::unique_lock lock(queueMutex_);
 if(stopping_.load(std::memory_order_relaxed) && scheduledWorkers_ == 0) {
  return;
 }
 stopping_.store(true, std::memory_order_relaxed);
 outbox_.request_stop();
 idleCv_.wait(lock, [this] { return scheduledWorkers_ == 0; });
 queue_.clear();
 activeBoxes_.clear();
 pendingCount_.store(0, std::memory_order_relaxed);
}
void LightingEngine::scheduleWorkersLocked() {
 const std::size_t desired = std::min<std::size_t>(workerLimit_, queue_.size() + activeBoxes_.size());
 while(!stopping_ && scheduledWorkers_ < desired) {
 ++scheduledWorkers_;
 try {
   if(!computePool_.submit([this] { runScheduledWork(); },
                           static_cast<int>(util::concurrent::TaskPriority::High))) {
    --scheduledWorkers_;
    if(scheduledWorkers_ == 0) {
     idleCv_.notify_all();
    }
    break;
   }
  } catch(...) {
   --scheduledWorkers_;
   throw;
  }
 }
}
bool LightingEngine::tryClaimBox(Box& out) {
 for(auto it = queue_.begin(); it != queue_.end(); ++it) {
  bool conflicts = false;
  for(const Box& active : activeBoxes_) {
   if(it->conflictsWith(active)) {
    conflicts = true;
    break;
   }
  }
  if(conflicts) {
   continue;
  }
  out = *it;
  queue_.erase(it);
  activeBoxes_.push_back(out);
  pendingCount_.store(queue_.size() + activeBoxes_.size(), std::memory_order_relaxed);
  return true;
 }
 return false;
}
void LightingEngine::releaseClaimedBoxLocked(const Box& box) {
 activeBoxes_.erase(std::remove_if(activeBoxes_.begin(),
                                   activeBoxes_.end(),
                                   [&](const Box& active) {
                                    return active.type == box.type && active.minX == box.minX &&
                                           active.minY == box.minY && active.minZ == box.minZ &&
                                           active.maxX == box.maxX && active.maxY == box.maxY &&
                                           active.maxZ == box.maxZ;
                                   }),
                    activeBoxes_.end());
 pendingCount_.store(queue_.size() + activeBoxes_.size(), std::memory_order_relaxed);
}
void LightingEngine::runScheduledWork() {
 Box box{LightType::Block, 0, 0, 0, 0, 0, 0};
 {
  const std::lock_guard lock(queueMutex_);
  if(stopping_ || !tryClaimBox(box)) {
   --scheduledWorkers_;
   if(scheduledWorkers_ == 0) {
    idleCv_.notify_all();
   }
   return;
  }
 }
 WorkerState state;
 try {
  runUpdate(box, state);
  } catch(...) {
  }
  releasePins(state);
  flushStaging();
  const std::lock_guard lock(queueMutex_);
 releaseClaimedBoxLocked(box);
 --scheduledWorkers_;
 scheduleWorkersLocked();
 if(scheduledWorkers_ == 0) {
  idleCv_.notify_all();
 }
}
Chunk* LightingEngine::chunkAt(int chunkX, int chunkZ, WorkerState& state) {
 const std::uint64_t key = chunkKey(chunkX, chunkZ);
 for(int slot = 0; slot < state.lastValidCount; ++slot) {
  if(state.lastKeys[slot] == key) {
   Chunk* chunk = state.lastChunks[slot];
   if(slot != 0) {
    // Promote to MRU so a repeating A,B,A,B... pattern stays a hit.
    std::swap(state.lastKeys[0], state.lastKeys[slot]);
    std::swap(state.lastChunks[0], state.lastChunks[slot]);
   }
   return chunk;
  }
 }
 Chunk* chunk = nullptr;
 if(const auto it = state.pinCache.find(key); it == state.pinCache.end()) {
  const std::lock_guard lock(registryMutex_);
  if(const auto reg = registry_.find(key); reg != registry_.end()) {
   chunk = reg->second;
  }
  if(chunk != nullptr && !chunk->tryAcquireRenderPin()) {
   chunk = nullptr;
  }
  state.pinCache.emplace(key, chunk);
 } else {
  chunk = it->second;
 }
 for(int slot = WorkerState::kCacheSlots - 1; slot > 0; --slot) {
  state.lastKeys[slot] = state.lastKeys[slot - 1];
  state.lastChunks[slot] = state.lastChunks[slot - 1];
 }
 state.lastKeys[0] = key;
 state.lastChunks[0] = chunk;
 if(state.lastValidCount < WorkerState::kCacheSlots) {
  ++state.lastValidCount;
 }
 return chunk;
}
void LightingEngine::releasePins(WorkerState& state) {
 for(const auto& [key, chunk] : state.pinCache) {
  (void)key;
  if(chunk != nullptr) {
   chunk->releaseRenderPin();
  }
 }
 state.pinCache.clear();
}
int LightingEngine::blockId(int x, int y, int z, WorkerState& state) {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000 || y < 0 || y >= Chunk::height) {
  return 0;
 }
 Chunk* chunk = chunkAt(x >> 4, z >> 4, state);
 return chunk != nullptr ? chunk->getBlockId(x & 15, y, z & 15) : 0;
}
int LightingEngine::brightness(LightType type, int x, int y, int z, WorkerState& state) {
 y = std::clamp(y, 0, Chunk::height - 1);
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
  return lightValue(type);
 }
 Chunk* chunk = chunkAt(x >> 4, z >> 4, state);
 return chunk != nullptr ? chunk->getLight(type, x & 15, y, z & 15) : 0;
}
void LightingEngine::setBrightness(LightType type, int x, int y, int z, int value, WorkerState& state) {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000 || y < 0 || y >= Chunk::height) {
  return;
 }
 Chunk* chunk = chunkAt(x >> 4, z >> 4, state);
 if(chunk == nullptr || chunk->getLight(type, x & 15, y, z & 15) == value) {
  return;
 }
 chunk->setLight(type, x & 15, y, z & 15, value);
}
bool LightingEngine::topY(int x, int y, int z, WorkerState& state) {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000 || y < 0) {
  return false;
 }
 if(y >= Chunk::height) {
  return true;
 }
 Chunk* chunk = chunkAt(x >> 4, z >> 4, state);
 return chunk != nullptr && chunk->isAboveMaxHeight(x & 15, y, z & 15);
}
void LightingEngine::queuePropagationBox(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 if(type == LightType::Sky && skyLightSuppressed_.load(std::memory_order_relaxed)) {
  return;
 }
 if(maxX < minX || maxY < minY || maxZ < minZ) {
  return;
 }
 if(maxX < -32000000 || minX >= 32000000 || maxZ < -32000000 || minZ > 32000000) {
  return;
 }
 minY = std::max(minY, 0);
 maxY = std::min(maxY, Chunk::height - 1);
 if(maxY < minY) {
  return;
 }
 push(type, minX, minY, minZ, maxX, maxY, maxZ, true);
}
void LightingEngine::runUpdate(const Box& update, WorkerState& state) {
 using block::Block;
 const LightType lightType = update.type;
 int minY = std::max(0, update.minY);
 int maxY = std::min(Chunk::height - 1, update.maxY);
 const int dx = update.maxX - update.minX + 1;
 const int dy = maxY - minY + 1;
 const int dz = update.maxZ - update.minZ + 1;
 const int volume = dx * dy * dz;
 if(volume > 32768) {
  if(dx >= dy && dx >= dz && dx > 1) {
   const int midX = update.minX + dx / 2;
   push(lightType, update.minX, minY, update.minZ, midX, maxY, update.maxZ, false);
   push(lightType, midX + 1, minY, update.minZ, update.maxX, maxY, update.maxZ, false);
  } else if(dy >= dx && dy >= dz && dy > 1) {
   const int midY = minY + dy / 2;
   push(lightType, update.minX, minY, update.minZ, update.maxX, midY, update.maxZ, false);
   push(lightType, update.minX, midY + 1, update.minZ, update.maxX, maxY, update.maxZ, false);
  } else if(dz > 1) {
   const int midZ = update.minZ + dz / 2;
   push(lightType, update.minX, minY, update.minZ, update.maxX, maxY, midZ, false);
   push(lightType, update.minX, minY, midZ + 1, update.maxX, maxY, update.maxZ, false);
  }
  return;
 }
 struct Spill {
  bool active = false;
  int minX = 0, minY = 0, minZ = 0, maxX = 0, maxY = 0, maxZ = 0;
  void include(int x, int y, int z) {
   if(!active) {
    active = true;
    minX = maxX = x;
    minY = maxY = y;
    minZ = maxZ = z;
    return;
   }
   minX = std::min(minX, x);
   minY = std::min(minY, y);
   minZ = std::min(minZ, z);
   maxX = std::max(maxX, x);
   maxY = std::max(maxY, y);
   maxZ = std::max(maxZ, z);
  }
 };
 Spill spill[6];
 bool anyChanged = false;
 bool changed = true;
 while(changed) {
  changed = false;
  int lastMinCx = 0;
  int lastMaxCx = 0;
  int lastMinCz = 0;
  int lastMaxCz = 0;
  bool lastLoaded = false;
  bool hasLast = false;
  for(int x = update.minX; x <= update.maxX; ++x) {
   for(int z = update.minZ; z <= update.maxZ; ++z) {
    const int minCx = (x - 1) >> 4;
    const int maxCx = (x + 1) >> 4;
    const int minCz = (z - 1) >> 4;
    const int maxCz = (z + 1) >> 4;
    bool loaded = false;
    if(hasLast && minCx == lastMinCx && maxCx == lastMaxCx && minCz == lastMinCz && maxCz == lastMaxCz) {
     loaded = lastLoaded;
    } else {
     loaded = true;
     for(int cx = minCx; cx <= maxCx && loaded; ++cx) {
      for(int cz = minCz; cz <= maxCz; ++cz) {
       if(chunkAt(cx, cz, state) == nullptr) {
        loaded = false;
        break;
       }
      }
     }
     lastMinCx = minCx;
     lastMaxCx = maxCx;
     lastMinCz = minCz;
     lastMaxCz = maxCz;
     lastLoaded = loaded;
     hasLast = true;
    }
    if(!loaded) {
     continue;
    }
    // The `loaded` test above proved every chunk in [minCx,maxCx]x[minCz,maxCz]
    // resolves non-null, and x/z are invariant across the whole y loop, so the
    // six neighbour samples per voxel need five chunk lookups per column rather
    // than nine per voxel.
    Chunk* const selfChunk = chunkAt(x >> 4, z >> 4, state);
    Chunk* const negXChunk = chunkAt((x - 1) >> 4, z >> 4, state);
    Chunk* const posXChunk = chunkAt((x + 1) >> 4, z >> 4, state);
    Chunk* const negZChunk = chunkAt(x >> 4, (z - 1) >> 4, state);
    Chunk* const posZChunk = chunkAt(x >> 4, (z + 1) >> 4, state);
    const int lx = x & 15;
    const int lz = z & 15;
    const int negLx = (x - 1) & 15;
    const int posLx = (x + 1) & 15;
    const int negLz = (z - 1) & 15;
    const int posLz = (z + 1) & 15;
    for(int y = minY; y <= maxY; ++y) {
     const int current = selfChunk->getLight(lightType, lx, y, lz);
     const int block = selfChunk->getBlockId(lx, y, lz);
     int opacity = Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(block)];
     if(opacity == 0) {
      opacity = 1;
     }
     int emission = 0;
     if(lightType == LightType::Sky) {
      if(selfChunk->isAboveMaxHeight(lx, y, lz)) {
       emission = 15;
      }
     } else {
      emission = lightRegistry_.blockEmission(block);
     }
     int newLight = 0;
     if(opacity < 15 || emission != 0) {
      int best = negXChunk->getLight(lightType, negLx, y, lz);
      best = std::max(best, posXChunk->getLight(lightType, posLx, y, lz));
      best = std::max(best, selfChunk->getLight(lightType, lx, std::max(y - 1, 0), lz));
      best = std::max(best, selfChunk->getLight(lightType, lx, std::min(y + 1, Chunk::height - 1), lz));
      best = std::max(best, negZChunk->getLight(lightType, lx, y, negLz));
      best = std::max(best, posZChunk->getLight(lightType, lx, y, posLz));
      best = std::max(0, best - opacity);
      newLight = std::max(best, emission);
     }
     if(current == newLight) {
      continue;
     }
     selfChunk->setLight(lightType, lx, y, lz, newLight);
     changed = true;
     anyChanged = true;
     if(x <= update.minX) {
      spill[0].include(x - 1, y, z);
     }
     if(y <= minY) {
      spill[1].include(x, y - 1, z);
     }
     if(z <= update.minZ) {
      spill[2].include(x, y, z - 1);
     }
     if(x >= update.maxX) {
      spill[3].include(x + 1, y, z);
     }
     if(y >= maxY) {
      spill[4].include(x, y + 1, z);
     }
     if(z >= update.maxZ) {
      spill[5].include(x, y, z + 1);
     }
    }
   }
  }
 }
 if(!anyChanged) {
  return;
 }
 for(const Spill& s : spill) {
  if(s.active) {
   queuePropagationBox(lightType, s.minX, s.minY, s.minZ, s.maxX, s.maxY, s.maxZ);
  }
 }
 publishDirtyRegion(DirtyRegion{update.minX, minY, update.minZ, update.maxX, maxY, update.maxZ});
}
void LightingEngine::publishDirtyRegion(DirtyRegion region) {
 mergeIntoPending(region);
}
double LightingEngine::distanceSqToCamera(const DirtyRegion& region) const noexcept {
 if(!cameraKnown_.load(std::memory_order_relaxed)) {
  return 0.0;
 }
 const double cameraX = cameraX_.load(std::memory_order_relaxed);
 const double cameraY = cameraY_.load(std::memory_order_relaxed);
 const double cameraZ = cameraZ_.load(std::memory_order_relaxed);
 const double dx = cameraX < region.minX ? region.minX - cameraX
                   : cameraX > region.maxX ? cameraX - region.maxX
                                          : 0.0;
 const double dy = cameraY < region.minY ? region.minY - cameraY
                   : cameraY > region.maxY ? cameraY - region.maxY
                                          : 0.0;
 const double dz = cameraZ < region.minZ ? region.minZ - cameraZ
                   : cameraZ > region.maxZ ? cameraZ - region.maxZ
                                          : 0.0;
 return dx * dx + dy * dy + dz * dz;
}
bool LightingEngine::isNearCamera(const DirtyRegion& region) const noexcept {
 if(!cameraKnown_.load(std::memory_order_relaxed)) {
  return false;
 }
 return distanceSqToCamera(region) <= kNearPublishDistance * kNearPublishDistance;
}
void LightingEngine::mergeIntoPending(DirtyRegion region) {
 const std::lock_guard lock(pendingMutex_);
 for(std::size_t i = 0; i < pending_.size();) {
  const DirtyRegion& existing = pending_[i];
  const bool touches = !(existing.maxX + 1 < region.minX || existing.minX > region.maxX + 1 ||
                         existing.maxY + 1 < region.minY || existing.minY > region.maxY + 1 ||
                         existing.maxZ + 1 < region.minZ || existing.minZ > region.maxZ + 1);
  if(!touches) {
   ++i;
   continue;
  }
  region.minX = std::min(region.minX, existing.minX);
  region.minY = std::min(region.minY, existing.minY);
  region.minZ = std::min(region.minZ, existing.minZ);
  region.maxX = std::max(region.maxX, existing.maxX);
  region.maxY = std::max(region.maxY, existing.maxY);
  region.maxZ = std::max(region.maxZ, existing.maxZ);
  pending_.erase(pending_.begin() + static_cast<std::ptrdiff_t>(i));
 }
 if(pending_.size() < kMaxPendingRegions) {
  pending_.push_back(region);
  return;
 }
 auto farthest = std::max_element(pending_.begin(), pending_.end(), [this](const DirtyRegion& a, const DirtyRegion& b) {
  return distanceSqToCamera(a) < distanceSqToCamera(b);
 });
 farthest->minX = std::min(farthest->minX, region.minX);
 farthest->minY = std::min(farthest->minY, region.minY);
 farthest->minZ = std::min(farthest->minZ, region.minZ);
 farthest->maxX = std::max(farthest->maxX, region.maxX);
 farthest->maxY = std::max(farthest->maxY, region.maxY);
 farthest->maxZ = std::max(farthest->maxZ, region.maxZ);
}
void LightingEngine::flushPending(bool includeFar) {
 const std::lock_guard lock(pendingMutex_);
 if(pending_.empty()) {
  return;
 }
 std::stable_sort(pending_.begin(), pending_.end(), [this](const DirtyRegion& a, const DirtyRegion& b) {
  return isNearCamera(a) && !isNearCamera(b);
 });
 std::vector<DirtyRegion> held;
 held.reserve(pending_.size());
 for(const DirtyRegion& region : pending_) {
  if((includeFar || isNearCamera(region)) && outbox_.tryPush(region)) {
   continue;
  }
  held.push_back(region);
 }
 pending_.swap(held);
}
} // namespace net::minecraft
