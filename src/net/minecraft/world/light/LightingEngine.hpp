#pragma once
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <vector>
#include "net/minecraft/world/LightType.hpp"
namespace net::minecraft {
class Chunk;
namespace world::light {
class UnifiedLightRegistry;
}
// Fully asynchronous lighting engine: propagation runs on one background thread; the main thread only enqueues via
// push() (never waits), drains finished regions via drainDirtyRegions() (the renderer re-meshes them), and
// registers/unregisters chunks (non-blocking). No public method blocks on the worker, so lighting never stalls a frame
// or gates world load. Chunk lifetime uses the shared render-pin: the worker pins each chunk it touches, so an evicting
// owner waits its turn through the existing non-blocking deferred render-eviction path.
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
  };
  void threadLoop(const std::stop_token& stop);
  void runUpdate(const Box& box);
  Chunk* chunkAt(int chunkX, int chunkZ);
  void releasePins();
  [[nodiscard]] int blockId(int x, int y, int z);
  [[nodiscard]] int brightness(LightType type, int x, int y, int z);
  void setBrightness(LightType type, int x, int y, int z, int value);
  [[nodiscard]] bool topY(int x, int y, int z);
  void queuePropagationBox(LightType type, int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
  [[nodiscard]] static constexpr std::uint64_t chunkKey(int chunkX, int chunkZ) noexcept {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32) |
           static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkZ));
  }
  mutable std::mutex queueMutex_;
  std::condition_variable workCv_;
  std::deque<Box> queue_;
  bool working_ = false;
  std::atomic<std::size_t> pendingCount_{0};
  std::atomic<bool> skyLightSuppressed_{false};
  std::mutex registryMutex_;
  std::unordered_map<std::uint64_t, Chunk*> registry_;
  std::unordered_map<std::uint64_t, Chunk*> pinCache_;
  mutable std::mutex outboxMutex_;
  std::vector<DirtyRegion> outbox_;
  world::light::UnifiedLightRegistry& lightRegistry_;
  std::jthread thread_;
};
} // namespace net::minecraft
