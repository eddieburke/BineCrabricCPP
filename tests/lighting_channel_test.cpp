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
  static void hold(LightingEngine& lighting, LightType type, int chunkX, int chunkZ) {
   const std::lock_guard lock(lighting.queueMutex_);
   LightingEngine::Box box{type, chunkX * 16, 0, chunkZ * 16, chunkX * 16 + 15, 127, chunkZ * 16 + 15};
   lighting.activeBoxes_.push_back(box);
   lighting.trackBoxLocked(box, true);
   lighting.pendingCount_.store(lighting.activeBoxes_.size());
  }
  static void finish(LightingEngine& lighting, LightType type, int chunkX, int chunkZ) {
   const std::lock_guard lock(lighting.queueMutex_);
   lighting.releaseClaimedBoxLocked({type, chunkX * 16, 0, chunkZ * 16, chunkX * 16 + 15, 127, chunkZ * 16 + 15});
  }
  static void publish(LightingEngine& lighting, LightingEngine::DirtyRegion region) {
   lighting.publishDirtyRegion(region);
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
TEST(LightingChannel, FarRegionsPublishWhileUnrelatedLightingRemainsBusy) {
 world::light::UnifiedLightRegistry registry;
 LightingEngine lighting(registry);
 LightingEngineTestAccess::stageFarRegion(lighting, LightingEngine::DirtyRegion{128, 64, 128, 128, 64, 128});
 LightingEngineTestAccess::setPendingWork(lighting, 1);
 const std::vector<LightingEngine::DirtyRegion> drained = lighting.drainDirtyRegions(1);
 ASSERT_EQ(drained.size(), 1U);
 EXPECT_EQ(drained.front().minX, 128);
 EXPECT_TRUE(lighting.busy());
 EXPECT_FALSE(lighting.hasDirtyRegions());
 lighting.stop();
}
TEST(LightingChannel, ColumnCompletionWaitsForBothChannelsAndPropagationButNotOtherColumns) {
 world::light::UnifiedLightRegistry registry;
 LightingEngine lighting(registry);
 lighting.awaitColumn(-1, 0);
 lighting.awaitColumn(80, 0);
 LightingEngineTestAccess::hold(lighting, LightType::Sky, -1, 0);
 LightingEngineTestAccess::hold(lighting, LightType::Block, -1, 0);
 LightingEngineTestAccess::hold(lighting, LightType::Block, 80, 0);
 EXPECT_TRUE(lighting.drainReadyColumns(10).empty());
 LightingEngineTestAccess::finish(lighting, LightType::Sky, -1, 0);
 EXPECT_TRUE(lighting.drainReadyColumns(10).empty());
 LightingEngineTestAccess::hold(lighting, LightType::Sky, -1, 0);
 LightingEngineTestAccess::finish(lighting, LightType::Block, -1, 0);
 EXPECT_TRUE(lighting.drainReadyColumns(10).empty());
 LightingEngineTestAccess::finish(lighting, LightType::Sky, -1, 0);
 const auto ready = lighting.drainReadyColumns(10);
 ASSERT_EQ(ready.size(), 1u);
 EXPECT_EQ(ready[0].x, -1);
 EXPECT_EQ(ready[0].z, 0);
 EXPECT_TRUE(lighting.busy());
 EXPECT_TRUE(lighting.drainReadyColumns(10).empty());
 LightingEngineTestAccess::finish(lighting, LightType::Block, 80, 0);
 const auto far = lighting.drainReadyColumns(10);
 ASSERT_EQ(far.size(), 1u);
 EXPECT_EQ(far[0].x, 80);
 EXPECT_FALSE(lighting.busy());
}
TEST(LightingChannel, UnchangedLocalWorkCompletesDuringAnUnrelatedFarBacklog) {
 world::light::UnifiedLightRegistry registry;
 LightingEngine lighting(registry);
 Chunk chunk(0, 0);
 lighting.registerChunk(&chunk);
 lighting.awaitColumn(0, 0);
 LightingEngineTestAccess::hold(lighting, LightType::Block, 80, 0);
 lighting.push(LightType::Block, 8, 64, 8, 8, 64, 8, false);
 std::vector<LightingEngine::ReadyColumn> ready;
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
 do {
  ready = lighting.drainReadyColumns(1);
  std::this_thread::yield();
 } while(ready.empty() && std::chrono::steady_clock::now() < deadline);
 EXPECT_EQ(ready.size(), 1u);
 EXPECT_TRUE(lighting.busy());
 EXPECT_FALSE(lighting.hasDirtyRegions());
 LightingEngineTestAccess::finish(lighting, LightType::Block, 80, 0);
 lighting.stop();
 lighting.unregisterChunk(&chunk);
}
TEST(LightingChannel, ReadyColumnsAreBoundedAndPrioritizeTheCamera) {
 world::light::UnifiedLightRegistry registry;
 LightingEngine lighting(registry);
 lighting.setCameraPosition(-1, 64, 0);
 for(const int x : {100, -1, 50}) lighting.awaitColumn(x, 0);
 EXPECT_TRUE(lighting.drainReadyColumns(0).empty());
 const auto first = lighting.drainReadyColumns(1);
 ASSERT_EQ(first.size(), 1u);
 EXPECT_EQ(first[0].x, -1);
 const auto rest = lighting.drainReadyColumns(20);
 ASSERT_EQ(rest.size(), 2u);
 EXPECT_EQ(rest[0].x, 50);
 EXPECT_EQ(rest[1].x, 100);
}
TEST(LightingChannel, StreamingDirtyRegionsStayLocalAndDoNotLoseCoverageAtTheMergeLimit) {
 world::light::UnifiedLightRegistry registry;
 LightingEngine lighting(registry);
 for(int x = 0; x < 1000; ++x)
  LightingEngineTestAccess::publish(lighting, {x * 16, 0, 0, x * 16 + 15, 127, 15});
 const auto regions = lighting.drainDirtyRegions(4096);
 int width = 0;
 for(const auto& region : regions) {
  EXPECT_LE(region.maxX - region.minX + 1, 64);
  width += region.maxX - region.minX + 1;
 }
 EXPECT_EQ(width, 16000);
 EXPECT_FALSE(lighting.hasDirtyRegions());
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
TEST(LightingChannel, BorderCellSkippedForMissingNeighbourRelightsWhenItRegisters) {
 util::concurrent::ThreadCoordinator::instance().configure(8, 2, {.maxComputeThreads = 8});
 constexpr int emitterId = 250;
 world::light::UnifiedLightRegistry::setBlockEmission(emitterId, 15);
 world::light::UnifiedLightRegistry registry;
 LightingEngine lighting(registry);
 std::vector<Chunk> chunks;
 chunks.reserve(5);
 chunks.emplace_back(0, 0);
 chunks.emplace_back(-1, 0);
 chunks.emplace_back(0, -1);
 chunks.emplace_back(0, 1);
 for(Chunk& chunk : chunks) {
  lighting.registerChunk(&chunk);
 }
 const auto settle = [&] {
  lighting.flushStaging();
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while(lighting.busy() && std::chrono::steady_clock::now() < deadline) {
   std::this_thread::yield();
  }
  EXPECT_FALSE(lighting.busy());
 };
 chunks[0].setBlock(15, 64, 8, emitterId);
 lighting.push(LightType::Block, 15, 64, 8, 15, 64, 8, false);
 settle();
 EXPECT_EQ(chunks[0].getLight(LightType::Block, 15, 64, 8), 0);
 chunks.emplace_back(1, 0);
 lighting.registerChunk(&chunks.back());
 settle();
 EXPECT_EQ(chunks[0].getLight(LightType::Block, 15, 64, 8), 15);
 EXPECT_EQ(chunks[0].getLight(LightType::Block, 14, 64, 8), 14);
 lighting.stop();
 for(Chunk& chunk : chunks) {
  lighting.unregisterChunk(&chunk);
 }
 world::light::UnifiedLightRegistry::setBlockEmission(emitterId, 0);
}
}
}
