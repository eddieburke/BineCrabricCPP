#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightingEngine.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft::test {
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
  std::this_thread::yield();
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
}
