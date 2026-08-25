#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightingEngine.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft {
class LightingEngineTestAccess {
 public:
  static void stageFarRegion(LightingEngine& lighting, LightingEngine::DirtyRegion region) {
   lighting.pending_.push_back(region);
  }
  static void setPendingWork(LightingEngine& lighting, std::size_t count) {
   lighting.pendingCount_.store(count, std::memory_order_relaxed);
  }
};
namespace test {
TEST(LightingChannel, ComputeTaskPublishesAndBoundedDrainConsumesDirtyRegion) {
 util::concurrent::ThreadCoordinator::instance().configure(8, 2, {.maxComputeThreads = 8});
 world::light::UnifiedLightRegistry registry;
 std::vector<Chunk> chunks;
 chunks.reserve(9);
 LightingEngine lighting(registry);
 for(int chunkX = -1; chunkX <= 1; ++chunkX) {
  for(int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
   chunks.emplace_back(chunkX, chunkZ);
   lighting.registerChunk(&chunks.back());
  }
 }
 Chunk& center = chunks[4];
 center.setLight(LightType::Block, 8, 64, 8, 15);
 lighting.push(LightType::Block, 8, 64, 8, 8, 64, 8, false);
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
 while(lighting.busy() && std::chrono::steady_clock::now() < deadline) {
  lighting.drainDirtyRegions(0);
 }
 ASSERT_FALSE(lighting.busy());
 EXPECT_TRUE(lighting.hasDirtyRegions());
 EXPECT_TRUE(lighting.drainDirtyRegions(0).empty());
 const std::vector<LightingEngine::DirtyRegion> drained = lighting.drainDirtyRegions(1);
 ASSERT_EQ(drained.size(), 1U);
 EXPECT_LE(drained.front().minX, 8);
 EXPECT_GE(drained.front().maxX, 8);
 EXPECT_LE(drained.front().minY, 64);
 EXPECT_GE(drained.front().maxY, 64);
 EXPECT_LE(drained.front().minZ, 8);
 EXPECT_GE(drained.front().maxZ, 8);
 EXPECT_FALSE(lighting.hasDirtyRegions());
 lighting.stop();
}
TEST(LightingChannel, FarRegionsWaitUntilAllLightingWorkSettles) {
 world::light::UnifiedLightRegistry registry;
 LightingEngine lighting(registry);
 LightingEngineTestAccess::stageFarRegion(lighting, LightingEngine::DirtyRegion{128, 64, 128, 128, 64, 128});
 LightingEngineTestAccess::setPendingWork(lighting, 1);
 EXPECT_TRUE(lighting.drainDirtyRegions(1).empty());
 EXPECT_TRUE(lighting.hasDirtyRegions());
 LightingEngineTestAccess::setPendingWork(lighting, 0);
 const std::vector<LightingEngine::DirtyRegion> drained = lighting.drainDirtyRegions(1);
 ASSERT_EQ(drained.size(), 1U);
 EXPECT_EQ(drained.front().minX, 128);
 EXPECT_FALSE(lighting.hasDirtyRegions());
 lighting.stop();
}
TEST(LightingChannel, CornerUpdateNeedsCardinalChunksOnly) {
 util::concurrent::ThreadCoordinator::instance().configure(8, 2, {.maxComputeThreads = 8});
 constexpr int emitterId = 250;
 world::light::UnifiedLightRegistry::setBlockEmission(emitterId, 15);
 world::light::UnifiedLightRegistry registry;
 LightingEngine lighting(registry);
 std::vector<Chunk> chunks;
 chunks.reserve(3);
 chunks.emplace_back(0, 0);
 chunks.emplace_back(1, 0);
 chunks.emplace_back(0, 1);
 for(Chunk& chunk : chunks) {
  lighting.registerChunk(&chunk);
 }
 chunks[0].setBlock(15, 64, 15, emitterId);
 lighting.push(LightType::Block, 15, 64, 15, 15, 64, 15, false);
 lighting.flushStaging();
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
 while(lighting.busy() && std::chrono::steady_clock::now() < deadline) {
  std::this_thread::yield();
 }
 EXPECT_FALSE(lighting.busy());
 EXPECT_EQ(chunks[0].getLight(LightType::Block, 15, 64, 15), 15);
 lighting.stop();
 for(Chunk& chunk : chunks) {
  lighting.unregisterChunk(&chunk);
 }
 world::light::UnifiedLightRegistry::setBlockEmission(emitterId, 0);
}
}
}
