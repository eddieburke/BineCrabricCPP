#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"
namespace {
struct ServerChunkCacheFixture {
 net::minecraft::server::MinecraftServer server;
 net::minecraft::EmptyWorldStorage storage;
 net::minecraft::ServerWorld world{&server, &storage, "test", 0, 1};
 net::minecraft::world::chunk::ChunkCache* cache = nullptr;
 ServerChunkCacheFixture() {
  world.setSpawnPos(net::minecraft::Vec3i{0, 64, 0});
  cache = world.chunkCache;
 }
};
class SpillingDecorator final : public net::minecraft::ChunkSource {
 public:
 net::minecraft::world::chunk::ChunkCache* cache = nullptr;
 int depth = 0;
 int deepest = 0;
 bool spilled = false;
 bool isChunkLoaded(int x, int z) const override { return cache->isChunkLoaded(x, z); }
 net::minecraft::Chunk& getChunk(int x, int z) override { return cache->getChunk(x, z); }
 net::minecraft::Chunk& loadChunk(int x, int z) override { return cache->loadChunk(x, z); }
 void decorate(net::minecraft::ChunkSource*, int, int) override {
  deepest = std::max(deepest, ++depth);
  if(!spilled) {
   spilled = true;
   cache->loadChunk(102, 100);
  }
  --depth;
 }
 bool save(bool, SaveProgressCallback) override { return true; }
 bool tick() override { return false; }
 bool canSave() const override { return true; }
 std::string getDebugInfo() const override { return {}; }
};
class RetryChunkStorage final : public net::minecraft::ChunkStorage {
 public:
 net::minecraft::Chunk loadChunk(net::minecraft::World* world, int x, int z) override {
  net::minecraft::Chunk chunk(world, x, z);
  chunk.empty = ++calls_ == 1;
  return chunk;
 }
 void saveChunk(net::minecraft::World*, net::minecraft::Chunk&) override {}
 void saveEntities(net::minecraft::World*, net::minecraft::Chunk&) override {}
 void tick() override {}
 void flush() override {}
 private:
 int calls_ = 0;
};
} // namespace
namespace net::minecraft::test {
TEST(ServerChunkCache, ChunkPosHashMatchesJava) {
 EXPECT_EQ(net::minecraft::chunkPosHashCode(0, 0), 0);
 EXPECT_EQ(net::minecraft::chunkPosHashCode(1, 1), (1 << 16) | 1);
 EXPECT_EQ(net::minecraft::chunkPosHashCode(-1, -1),
           std::numeric_limits<int>::min() | (32767 << 16) | 32768 | 32767);
 EXPECT_EQ(net::minecraft::chunkPosHashCode(32767, 32767), (32767 << 16) | 32767);
 EXPECT_EQ(net::minecraft::chunkPosHashCode(-32768, 0), std::numeric_limits<int>::min());
}
TEST(ServerChunkCache, IsLoadedUsesSpawnRadius) {
 ServerChunkCacheFixture fixture;
 ASSERT_NE(fixture.cache, nullptr);
 fixture.cache->forceLoad = true;
 fixture.cache->loadChunk(5, 5);
 EXPECT_TRUE(fixture.cache->isChunkLoaded(5, 5));
 fixture.cache->dropChunk(5, 5);
 EXPECT_NE(fixture.cache->getDebugInfo().find("unload: 0"), std::string::npos);
 fixture.cache->loadChunk(12, 12);
 EXPECT_TRUE(fixture.cache->isChunkLoaded(12, 12));
 fixture.cache->dropChunk(12, 12);
 EXPECT_NE(fixture.cache->getDebugInfo().find("unload: 1"), std::string::npos);
}
TEST(ServerChunkCache, GetChunkReturnsEmptyWhenEventsDisabled) {
 ServerChunkCacheFixture fixture;
 ASSERT_NE(fixture.cache, nullptr);
 fixture.world.setEventProcessingEnabled(false);
 fixture.cache->forceLoad = false;
 net::minecraft::Chunk& chunk = fixture.cache->getChunk(3, 3);
 EXPECT_TRUE(chunk.empty);
 EXPECT_FALSE(fixture.cache->isChunkLoaded(3, 3));
}
TEST(ServerChunkCache, ForceLoadBypassesEventGate) {
 ServerChunkCacheFixture fixture;
 ASSERT_NE(fixture.cache, nullptr);
 fixture.world.setEventProcessingEnabled(false);
 fixture.cache->forceLoad = true;
 net::minecraft::Chunk& chunk = fixture.cache->loadChunk(4, 4);
 EXPECT_FALSE(chunk.empty);
 EXPECT_TRUE(fixture.cache->isChunkLoaded(4, 4));
}
TEST(ServerChunkCache, CanSaveRespectsSavingDisabled) {
 ServerChunkCacheFixture fixture;
 ASSERT_NE(fixture.cache, nullptr);
 EXPECT_TRUE(fixture.cache->canSave());
 fixture.world.savingDisabled = true;
 EXPECT_FALSE(fixture.cache->canSave());
}
TEST(ServerChunkCache, TickUnloadsMarkedChunks) {
 ServerChunkCacheFixture fixture;
 ASSERT_NE(fixture.cache, nullptr);
 fixture.cache->forceLoad = true;
 fixture.cache->loadChunk(20, 20);
 EXPECT_TRUE(fixture.cache->isChunkLoaded(20, 20));
 fixture.cache->dropChunk(20, 20);
 fixture.cache->tick();
 EXPECT_FALSE(fixture.cache->isChunkLoaded(20, 20));
}
TEST(ServerChunkCache, ResidencyPlanIsCircularAndStableUntilInputsChange) {
 ServerChunkCacheFixture fixture;
 ASSERT_NE(fixture.cache, nullptr);
 fixture.cache->setActiveRadius(3);
 fixture.cache->setChunkCacheCenter(-10, 7);
 EXPECT_EQ(fixture.cache->desiredChunkCount(), 29U);
 const std::size_t queued = fixture.cache->queuedResidencyCount();
 fixture.cache->setChunkCacheCenter(-10, 7);
 EXPECT_EQ(fixture.cache->queuedResidencyCount(), queued);
 fixture.cache->setChunkCacheCenter(-9, 7);
 EXPECT_EQ(fixture.cache->desiredChunkCount(), 29U);
}
TEST(ServerChunkCache, ResidencyPlanSkipsResidentChunksAndUnloadsTheTrailingEdge) {
 ServerChunkCacheFixture fixture;
 ASSERT_NE(fixture.cache, nullptr);
 fixture.cache->forceLoad = true;
 fixture.cache->setActiveRadius(3);
 fixture.cache->setChunkCacheCenter(100, 100);
 EXPECT_EQ(fixture.cache->queuedResidencyCount(), 29U);
 fixture.cache->loadChunk(97, 100);
 fixture.cache->loadChunk(100, 100);
 fixture.cache->loadChunk(103, 100);
 fixture.cache->setChunkCacheCenter(101, 100);
 EXPECT_EQ(fixture.cache->desiredChunkCount(), 29U);
 EXPECT_EQ(fixture.cache->queuedResidencyCount(), 27U);
 EXPECT_NE(fixture.cache->getDebugInfo().find("Pending unload: 1"), std::string::npos);
 fixture.cache->tick();
 EXPECT_FALSE(fixture.cache->isChunkLoaded(97, 100));
 EXPECT_TRUE(fixture.cache->isChunkLoaded(100, 100));
 EXPECT_TRUE(fixture.cache->isChunkLoaded(103, 100));
 EXPECT_EQ(fixture.cache->loadedChunkCount(), 2U);
 EXPECT_TRUE(fixture.cache->save(true));
}
TEST(ServerChunkCache, SynchronousLoadsDecorateAsSoonAsNeighborsArrive) {
 ServerChunkCacheFixture fixture;
 ASSERT_NE(fixture.cache, nullptr);
 fixture.cache->forceLoad = true;
 fixture.cache->loadChunk(40, 40);
 fixture.cache->loadChunk(41, 40);
 fixture.cache->loadChunk(40, 41);
 EXPECT_FALSE(fixture.cache->getChunk(40, 40).terrainPopulated);
 fixture.cache->loadChunk(41, 41);
 EXPECT_TRUE(fixture.cache->getChunk(40, 40).terrainPopulated);
}
TEST(ServerChunkCache, AsyncLoadsDecorateThroughTheTickBudget) {
 ServerChunkCacheFixture fixture;
 ASSERT_NE(fixture.cache, nullptr);
 fixture.cache->forceLoad = true;
 fixture.cache->setActiveRadius(3);
 fixture.cache->setChunkCacheCenter(60, 60);
 for(const auto [x, z] : {std::pair{60, 60}, std::pair{61, 60}, std::pair{60, 61}, std::pair{61, 61}}) {
  fixture.cache->requestChunkAsync(x, z);
 }
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
 while(std::chrono::steady_clock::now() < deadline &&
       !(fixture.cache->isChunkLoaded(60, 60) && fixture.cache->getChunk(60, 60).terrainPopulated)) {
  fixture.cache->tick();
  std::this_thread::yield();
 }
 ASSERT_TRUE(fixture.cache->isChunkLoaded(60, 60));
 EXPECT_TRUE(fixture.cache->getChunk(60, 60).terrainPopulated);
}
TEST(ServerChunkCache, DecorationSpillingIntoAnUnloadedChunkDoesNotCascade) {
 ServerChunkCacheFixture fixture;
 SpillingDecorator decorator;
 world::chunk::ChunkCache cache(&fixture.world, nullptr, &decorator);
 decorator.cache = &cache;
 cache.forceLoad = true;
 for(const auto [x, z] : {std::pair{100, 100}, std::pair{101, 100}, std::pair{100, 101}, std::pair{102, 101}}) {
  cache.loadChunk(x, z);
 }
 cache.loadChunk(101, 101);
 EXPECT_EQ(decorator.deepest, 1);
 EXPECT_TRUE(cache.getChunk(100, 100).terrainPopulated);
 EXPECT_FALSE(cache.getChunk(101, 100).terrainPopulated);
 cache.tick();
 EXPECT_EQ(decorator.deepest, 1);
 EXPECT_TRUE(cache.getChunk(101, 100).terrainPopulated);
}
TEST(ServerChunkCache, LightNeutralBlockSwapQueuesNoLighting) {
 ServerChunkCacheFixture fixture;
 ASSERT_NE(fixture.cache, nullptr);
 fixture.cache->forceLoad = true;
 net::minecraft::Chunk& chunk = fixture.cache->loadChunk(2, 2);
 chunk.setBlock(4, 10, 4, 1);
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
 while(fixture.world.hasPendingLightingUpdates() && std::chrono::steady_clock::now() < deadline) {
  fixture.world.doLightingUpdates(std::numeric_limits<std::size_t>::max());
  std::this_thread::yield();
 }
 ASSERT_FALSE(fixture.world.hasPendingLightingUpdates());
 EXPECT_TRUE(chunk.setBlock(4, 10, 4, 16));
 EXPECT_FALSE(fixture.world.hasPendingLightingUpdates());
 EXPECT_EQ(chunk.getBlockId(4, 10, 4), 16);
 EXPECT_TRUE(chunk.setBlock(4, 10, 4, 0));
 EXPECT_TRUE(fixture.world.hasPendingLightingUpdates());
}
TEST(ServerChunkCache, AsyncLoadWindowScalesWithComputeWorkers) {
 EXPECT_EQ(world::chunk::ChunkCache::asyncLoadWindow(1), 64U);
 EXPECT_EQ(world::chunk::ChunkCache::asyncLoadWindow(3), 96U);
 EXPECT_EQ(world::chunk::ChunkCache::asyncLoadWindow(8), 256U);
 EXPECT_EQ(world::chunk::ChunkCache::asyncLoadWindow(64), 256U);
}
TEST(ServerChunkCache, FailedSynchronousLoadDoesNotPermanentlyCacheTheEmptyChunk) {
 ServerChunkCacheFixture fixture;
 world::chunk::ChunkCache cache(&fixture.world, std::make_unique<RetryChunkStorage>(), nullptr);
 EXPECT_TRUE(cache.loadChunk(37, -21).isEmpty());
 EXPECT_FALSE(cache.isChunkLoaded(37, -21));
 EXPECT_FALSE(cache.loadChunk(37, -21).isEmpty());
 EXPECT_TRUE(cache.isChunkDataReady(37, -21));
 cache.unloadChunk(37, -21);
}
} // namespace net::minecraft::test
