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
class LightingEngineTestAccess;
class LightingEngine {
 public:
 struct DirtyRegion {
  int minX, minY, minZ, maxX, maxY, maxZ;
 };
 struct TraceStats {
  std::size_t stagedWork = 0;
  std::size_t pendingWork = 0;
  std::size_t pendingRegions = 0;
  std::size_t publishedRegions = 0;
 };
 explicit LightingEngine(world::light::UnifiedLightRegistry& registry);
 ~LightingEngine() {
  stop();
 }
 LightingEngine(const LightingEngine&) = delete;
 LightingEngine& operator=(const LightingEngine&) = delete;
 void push(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool merge);
 void flushStaging();
 void setSkyLightSuppressed(bool suppressed) noexcept {
  skyLightSuppressed_.store(suppressed, std::memory_order_relaxed);
 }
 void setCameraPosition(double x, double y, double z) noexcept {
  cameraX_.store(x, std::memory_order_relaxed);
  cameraY_.store(y, std::memory_order_relaxed);
  cameraZ_.store(z, std::memory_order_relaxed);
  cameraKnown_.store(true, std::memory_order_relaxed);
 }

 void registerChunk(Chunk* chunk);
 void unregisterChunk(Chunk* chunk);
 [[nodiscard]] std::vector<DirtyRegion> drainDirtyRegions(std::size_t maxRegions);
 [[nodiscard]] bool hasDirtyRegions() const;
 [[nodiscard]] TraceStats traceStats() const;
 [[nodiscard]] bool busy() const noexcept {
  return pendingCount_.load(std::memory_order_relaxed) + stagedCount_.load(std::memory_order_relaxed) != 0;
 }
 void stop();

 private:
  friend class LightingEngineTestAccess;
 struct Box {
  LightType type;
  int minX, minY, minZ, maxX, maxY, maxZ;
  bool expand(int x0, int y0, int z0, int x1, int y1, int z1);
  void cover(int x0, int y0, int z0, int x1, int y1, int z1);
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
  // Flood-fill propagation visits blocks in near-spatial order, so almost
  // every chunkAt() call in a run repeats the previous chunk (confirmed via
  // VTune: chunkAt/chunkKey were ~10% of a viewDistance-20 streaming trace).
  // Two slots, not one: runUpdate samples the 6 cardinal neighbors of every
  // voxel, and at an x/z chunk edge those alternate between exactly two
  // chunks (…A, B, A, A, A, A, A…) — a single-entry cache would miss on
  // every other lookup there. Slot 0 is the most-recently-used.
  static constexpr int kCacheSlots = 2;
  std::uint64_t lastKeys[kCacheSlots] = {};
  Chunk* lastChunks[kCacheSlots] = {};
  int lastValidCount = 0;
 };
 void scheduleWorkersLocked();
 void runScheduledWork();
 [[nodiscard]] bool tryClaimBox(Box& out);
 void releaseClaimedBoxLocked(const Box& box);
 void publishDirtyRegion(DirtyRegion region);
 void mergeIntoPending(DirtyRegion region);
  void flushPending(bool includeFar);
 [[nodiscard]] double distanceSqToCamera(const DirtyRegion& region) const noexcept;
 [[nodiscard]] bool isNearCamera(const DirtyRegion& region) const noexcept;
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


 static constexpr std::size_t kStagingFlushBoxes = 1024;
 static constexpr std::size_t kStagingMergeScan = 32;
 static constexpr std::size_t kMaxPendingRegions = 64;
  static constexpr double kNearPublishDistance = 64.0;
 std::mutex stagingMutex_;
 std::deque<Box> staging_;
 mutable std::mutex queueMutex_;
 std::condition_variable idleCv_;
 std::deque<Box> queue_;
 std::vector<Box> activeBoxes_;
 std::size_t scheduledWorkers_ = 0;
 std::atomic<bool> stopping_{false};
 std::atomic<std::size_t> pendingCount_{0};
 std::atomic<std::size_t> stagedCount_{0};
 std::atomic<bool> skyLightSuppressed_{false};
 std::atomic<double> cameraX_{0.0};
 std::atomic<double> cameraY_{0.0};
 std::atomic<double> cameraZ_{0.0};
 std::atomic<bool> cameraKnown_{false};
 mutable std::mutex pendingMutex_;
 std::vector<DirtyRegion> pending_;
  std::mutex registryMutex_;
 std::unordered_map<std::uint64_t, Chunk*> registry_;
 util::concurrent::Channel<DirtyRegion> outbox_{4096};
 world::light::UnifiedLightRegistry& lightRegistry_;
 util::concurrent::WorkerPool& computePool_;
 unsigned workerLimit_;
};
} // namespace net::minecraft
