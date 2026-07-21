#include "net/minecraft/world/light/LightingEngine.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#include <algorithm>
#include <utility>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
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
    : lightRegistry_(registry), thread_([this](const std::stop_token& stop) { threadLoop(stop); }) {
}
void LightingEngine::push(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool merge) {
 {
  const std::lock_guard lock(queueMutex_);
  if(merge) {
   const std::size_t n = std::min<std::size_t>(queue_.size(), 5);
   for(std::size_t i = 0; i < n; ++i) {
    Box& existing = queue_[queue_.size() - i - 1];
    if(existing.type == type && existing.expand(minX, minY, minZ, maxX, maxY, maxZ)) {
     return;
    }
   }
  }
  // Bound memory without ever dropping coverage: once the backlog is huge, fold new work into the most recent box
  // of the same type (it over-covers and is re-split by runUpdate) rather than discarding queued regions.
  constexpr std::size_t kMaxQueue = 200000;
  if(queue_.size() >= kMaxQueue) {
   for(auto it = queue_.rbegin(); it != queue_.rend(); ++it) {
    if(it->type == type) {
     it->cover(minX, minY, minZ, maxX, maxY, maxZ);
     pendingCount_.store(queue_.size() + (working_ ? 1U : 0U), std::memory_order_relaxed);
     return;
    }
   }
  }
  const std::int64_t volume = static_cast<std::int64_t>(maxX - minX + 1) *
                              static_cast<std::int64_t>(maxY - minY + 1) *
                              static_cast<std::int64_t>(maxZ - minZ + 1);
  if(volume <= 64) {
   queue_.push_front(Box{type, minX, minY, minZ, maxX, maxY, maxZ});
  } else {
   queue_.push_back(Box{type, minX, minY, minZ, maxX, maxY, maxZ});
  }
  pendingCount_.store(queue_.size() + (working_ ? 1U : 0U), std::memory_order_relaxed);
 }
 workCv_.notify_one();
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
 const std::lock_guard lock(outboxMutex_);
 if(maxRegions == 0 || outbox_.empty()) {
  return {};
 }
 if(maxRegions >= outbox_.size()) {
  return std::exchange(outbox_, {});
 }
 const std::size_t count = std::min(maxRegions, outbox_.size());
 auto first = outbox_.end() - static_cast<std::ptrdiff_t>(count);
 std::vector<DirtyRegion> regions(first, outbox_.end());
 outbox_.erase(first, outbox_.end());
 return regions;
}
bool LightingEngine::hasDirtyRegions() const {
 const std::lock_guard lock(outboxMutex_);
 return !outbox_.empty();
}
void LightingEngine::stop() {
 if(!thread_.joinable()) {
  return;
 }
 thread_.request_stop();
 workCv_.notify_all();
 thread_.join();
 const std::lock_guard lock(queueMutex_);
 queue_.clear();
 pendingCount_.store(0, std::memory_order_relaxed);
}
void LightingEngine::threadLoop(const std::stop_token& stop) {
 for(;;) {
  Box box{LightType::Block, 0, 0, 0, 0, 0, 0};
  {
   std::unique_lock lock(queueMutex_);
   workCv_.wait(lock, [&] { return stop.stop_requested() || !queue_.empty(); });
   if(stop.stop_requested()) {
    return;
   }
   box = queue_.front();
   queue_.pop_front();
   working_ = true;
   pendingCount_.store(queue_.size() + 1, std::memory_order_relaxed);
  }
  runUpdate(box);
  releasePins();
  {
   const std::lock_guard lock(queueMutex_);
   working_ = false;
   pendingCount_.store(queue_.size(), std::memory_order_relaxed);
  }
 }
}
Chunk* LightingEngine::chunkAt(int chunkX, int chunkZ) {
 const std::uint64_t key = chunkKey(chunkX, chunkZ);
 if(const auto it = pinCache_.find(key); it != pinCache_.end()) {
  return it->second;
 }
 Chunk* chunk = nullptr;
 {
  const std::lock_guard lock(registryMutex_);
  if(const auto reg = registry_.find(key); reg != registry_.end()) {
   chunk = reg->second;
  }
 }
 // Pin against eviction; a chunk mid-eviction reads as absent so the owner can free it without ever waiting on us.
 if(chunk != nullptr && !chunk->tryAcquireRenderPin()) {
  chunk = nullptr;
 }
 pinCache_.emplace(key, chunk);
 return chunk;
}
void LightingEngine::releasePins() {
 for(const auto& [key, chunk] : pinCache_) {
  if(chunk != nullptr) {
   chunk->releaseRenderPin();
  }
 }
 pinCache_.clear();
}
int LightingEngine::blockId(int x, int y, int z) {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000 || y < 0 || y >= Chunk::height) {
  return 0;
 }
 Chunk* chunk = chunkAt(x >> 4, z >> 4);
 return chunk != nullptr ? chunk->getBlockId(x & 15, y, z & 15) : 0;
}
int LightingEngine::brightness(LightType type, int x, int y, int z) {
 y = std::clamp(y, 0, Chunk::height - 1);
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
  return lightValue(type);
 }
 Chunk* chunk = chunkAt(x >> 4, z >> 4);
 return chunk != nullptr ? chunk->getLight(type, x & 15, y, z & 15) : 0;
}
void LightingEngine::setBrightness(LightType type, int x, int y, int z, int value) {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000 || y < 0 || y >= Chunk::height) {
  return;
 }
 Chunk* chunk = chunkAt(x >> 4, z >> 4);
 if(chunk == nullptr || chunk->getLight(type, x & 15, y, z & 15) == value) {
  return;
 }
 chunk->setLight(type, x & 15, y, z & 15, value);
}
bool LightingEngine::topY(int x, int y, int z) {
 if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000 || y < 0) {
  return false;
 }
 if(y >= Chunk::height) {
  return true;
 }
 Chunk* chunk = chunkAt(x >> 4, z >> 4);
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
void LightingEngine::runUpdate(const Box& update) {
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
 // Spill accumulators for light escaping each face of the update box:
 // -X, -Y, -Z, +X, +Y, +Z.
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
  int lastCx = 0;
  int lastCz = 0;
  bool lastLoaded = false;
  for(int x = update.minX; x <= update.maxX; ++x) {
   for(int z = update.minZ; z <= update.maxZ; ++z) {
    bool loaded = false;
    if(lastLoaded && (x >> 4) == lastCx && (z >> 4) == lastCz) {
     loaded = true;
    } else {
     loaded = true;
     for(int cx = (x - 1) >> 4; cx <= (x + 1) >> 4 && loaded; ++cx) {
      for(int cz = (z - 1) >> 4; cz <= (z + 1) >> 4; ++cz) {
       if(chunkAt(cx, cz) == nullptr) {
        loaded = false;
        break;
       }
      }
     }
     lastCx = x >> 4;
     lastCz = z >> 4;
     lastLoaded = loaded;
    }
    if(!loaded) {
     continue;
    }
    for(int y = minY; y <= maxY; ++y) {
     const int current = brightness(lightType, x, y, z);
     const int block = blockId(x, y, z);
     int opacity = Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(block)];
     if(opacity == 0) {
      opacity = 1;
     }
     int emission = 0;
     if(lightType == LightType::Sky) {
      if(topY(x, y, z)) {
       emission = 15;
      }
     } else {
      emission = lightRegistry_.blockEmission(block);
     }
     int newLight = 0;
     if(opacity < 15 || emission != 0) {
      int best = brightness(lightType, x - 1, y, z);
      best = std::max(best, brightness(lightType, x + 1, y, z));
      best = std::max(best, brightness(lightType, x, y - 1, z));
      best = std::max(best, brightness(lightType, x, y + 1, z));
      best = std::max(best, brightness(lightType, x, y, z - 1));
      best = std::max(best, brightness(lightType, x, y, z + 1));
      best = std::max(0, best - opacity);
      newLight = std::max(best, emission);
     }
     if(current == newLight) {
      continue;
     }
     setBrightness(lightType, x, y, z, newLight);
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
 const std::lock_guard lock(outboxMutex_);
 constexpr std::size_t maxOutbox = 4096;
 if(!outbox_.empty()) {
  DirtyRegion& last = outbox_.back();
  const bool overlapsOrTouches = last.maxX + 1 >= update.minX && update.maxX + 1 >= last.minX &&
                                 last.maxY + 1 >= minY && maxY + 1 >= last.minY && last.maxZ + 1 >= update.minZ &&
                                 update.maxZ + 1 >= last.minZ;
  if(overlapsOrTouches || outbox_.size() >= maxOutbox) {
   last.minX = std::min(last.minX, update.minX);
   last.minY = std::min(last.minY, minY);
   last.minZ = std::min(last.minZ, update.minZ);
   last.maxX = std::max(last.maxX, update.maxX);
   last.maxY = std::max(last.maxY, maxY);
   last.maxZ = std::max(last.maxZ, update.maxZ);
   return;
  }
 }
 outbox_.push_back(DirtyRegion{update.minX, minY, update.minZ, update.maxX, maxY, update.maxZ});
}
} // namespace net::minecraft
