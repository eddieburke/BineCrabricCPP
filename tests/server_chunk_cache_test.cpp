#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
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
