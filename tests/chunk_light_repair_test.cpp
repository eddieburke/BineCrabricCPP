#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"

namespace net::minecraft::test {
TEST(ChunkLightRepair, LoadRepairsStaleLightAndRevisitsArrivingBorders) {
 struct RepairWorld final : World {
  RepairWorld() : World("light-repair", 12345) {
   setChunkCache(nullptr);
  }
 } world;
 auto addChunk = [&world](int x, int z) -> Chunk& {
  Chunk& chunk = world.chunks().try_emplace(ChunkPos{x, z}, &world, x, z).first->second;
  chunk.populateHeightMapOnly();
  chunk.load();
  return chunk;
 };
 auto settle = [&world] {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while(world.doLightingUpdates() && std::chrono::steady_clock::now() < deadline) {
   std::this_thread::yield();
  }
  return !world.hasPendingLightingUpdates();
 };
 Chunk& center = addChunk(0, 0);
 center.setLight(LightType::Block, 8, 64, 8, 15);
 center.setLight(LightType::Sky, 8, 64, 8, 0);
 center.blocks[(15 << 11) | (8 << 7) | 64] = static_cast<std::uint8_t>(Block::TORCH->id);
 center.blocks[(8 << 11) | (8 << 7) | 63] = static_cast<std::uint8_t>(Block::STONE->id);
 center.setLight(LightType::Sky, 8, 63, 8, 15);
 center.populateHeightMapOnly();
 ASSERT_TRUE(settle());
 EXPECT_EQ(center.getLight(LightType::Block, 15, 64, 8), 0);
 for(int x = -1; x <= 1; ++x) {
  for(int z = -1; z <= 1; ++z) {
   if(x != 0 || z != 0) {
    addChunk(x, z);
   }
  }
 }
 ASSERT_TRUE(settle());
 EXPECT_EQ(center.getLight(LightType::Block, 8, 64, 8), 7);
 EXPECT_EQ(center.getLight(LightType::Sky, 8, 64, 8), 15);
 EXPECT_EQ(center.getLight(LightType::Sky, 8, 63, 8), 0);
 EXPECT_EQ(center.getLight(LightType::Block, 15, 64, 8), 14);
 EXPECT_EQ(world.getChunk(1, 0).getLight(LightType::Block, 0, 64, 8), 13);
 EXPECT_TRUE(center.dirty.load());
 EXPECT_EQ(center.getBlockId(15, 64, 8), Block::TORCH->id);
 EXPECT_EQ(center.getBlockId(8, 63, 8), Block::STONE->id);
}
}
