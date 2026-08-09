#pragma once
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "net/minecraft/util/concurrent/Channel.hpp"
#include "net/minecraft/world/light/LightType.hpp"
namespace net::minecraft {
class Chunk;
namespace util::concurrent {
class WorkerPool;
}
namespace world::light {
class UnifiedLightRegistry;
}
// Fully asynchronous lighting engine: propagation runs on background worker
// threads (sharded by non-overlapping boxes); the main thread only enqueues via
// push() (never waits), drains finished regions via drainDirtyRegions(), and
// registers/unregisters chunks (non-blocking). Chunk lifetime uses the shared
// render lease: workers lease each chunk they touch, and eviction tombstones
// the chunk and defers destruction until the leases drain (ChunkCache graveyard).
class LightingEngine {
 public:
 struct DirtyRegion {
  int minX, minY, minZ, maxX, maxY, maxZ;
 };
 explicit LightingEngine(world::light::UnifiedLightRegistry& registry);
 ~LightingEngine() {
  stop();
 }
 LightingEngine(const LightingEngine&) = delete;
 LightingEngine& operator=(const LightingEngine&) = delete;
 void push(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool merge);
 void setSkyLightSuppressed(bool suppressed) noexcept {
  skyLightSuppressed_.store(suppressed, std::memory_order_relaxed);
 }
 void registerChunk(Chunk* chunk);
 void unregisterChunk(Chunk* chunk);
 [[nodiscard]] std::vector<DirtyRegion> drainDirtyRegions(std::size_t maxRegions);
 [[nodiscard]] bool hasDirtyRegions() const;
 [[nodiscard]] bool busy() const noexcept {
  return pendingCount_.load(std::memory_order_relaxed) != 0;
 }
 void stop();

 private:
 struct Box {
  LightType type;
  int minX, minY, minZ, maxX, maxY, maxZ;
  bool expand(int x0, int y0, int z0, int x1, int y1, int z1);
  void cover(int x0, int y0, int z0, int x1, int y1, int z1);
  // Conflict test for concurrent claiming. Different light types write
  // disjoint nibble arrays and never conflict. Same-type boxes need a
  // 1-block margin: light nibbles pack two Y-adjacent cells per byte, so
  // merely adjacent boxes would race read-modify-writes on shared bytes.
  [[nodiscard]] bool conflictsWith(const Box& other) const noexcept {
   if(type != other.type) {
    return false;
   }
   return !(maxX + 1 < other.minX || minX > other.maxX + 1 || maxY + 1 < other.minY || minY > other.maxY + 1 ||
            maxZ + 1 < other.minZ || minZ > other.maxZ + 1);
  }
 };
 struct WorkerState {
  std::unordered_map<std::uint64_t, Chunk*> pinCache;
 };
 void scheduleWorkersLocked();
 void runScheduledWork();
 [[nodiscard]] bool tryClaimBox(Box& out);
 void releaseClaimedBoxLocked(const Box& box);
 void publishDirtyRegion(DirtyRegion region);
 void runUpdate(const Box& box, WorkerState& state);
 Chunk* chunkAt(int chunkX, int chunkZ, WorkerState& state);
 void releasePins(WorkerState& state);
 [[nodiscard]] int blockId(int x, int y, int z, WorkerState& state);
 [[nodiscard]] int brightness(LightType type, int x, int y, int z, WorkerState& state);
 void setBrightness(LightType type, int x, int y, int z, int value, WorkerState& state);
 [[nodiscard]] bool topY(int x, int y, int z, WorkerState& state);
 void queuePropagationBox(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
 [[nodiscard]] static constexpr std::uint64_t chunkKey(int chunkX, int chunkZ) noexcept {
  return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32) |
         static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkZ));
 }
 mutable std::mutex queueMutex_;
 std::condition_variable idleCv_;
 std::deque<Box> queue_;
 std::vector<Box> activeBoxes_;
 std::size_t scheduledWorkers_ = 0;
 bool stopping_ = false;
 std::atomic<std::size_t> pendingCount_{0};
 std::atomic<bool> skyLightSuppressed_{false};
 std::mutex registryMutex_;
 std::unordered_map<std::uint64_t, Chunk*> registry_;
 util::concurrent::Channel<DirtyRegion> outbox_{4096};
 world::light::UnifiedLightRegistry& lightRegistry_;
 util::concurrent::WorkerPool& computePool_;
 unsigned workerLimit_;
};
} // namespace net::minecraft
