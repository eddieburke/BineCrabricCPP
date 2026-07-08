#include <gtest/gtest.h>

#include <limits>
#include <string>

#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/world/chunk/ServerChunkCache.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"

namespace {
struct ServerChunkCacheFixture {
    net::minecraft::server::MinecraftServer server;
    net::minecraft::EmptyWorldStorage storage;
    net::minecraft::ServerWorld world{&server, &storage, "test", 0, 1};
    net::minecraft::server::world::chunk::ServerChunkCache* cache = nullptr;

    ServerChunkCacheFixture() {
        world.setSpawnPos(net::minecraft::Vec3i{0, 64, 0});
        cache = world.chunkCache;
    }
};
}  // namespace

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
    fixture.cache->isLoaded(5, 5);
    EXPECT_NE(fixture.cache->getDebugInfo().find("Drop: 0"), std::string::npos);
    fixture.cache->loadChunk(12, 12);
    EXPECT_TRUE(fixture.cache->isChunkLoaded(12, 12));
    fixture.cache->isLoaded(12, 12);
    EXPECT_NE(fixture.cache->getDebugInfo().find("Drop: 1"), std::string::npos);
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
    fixture.cache->isLoaded(20, 20);
    fixture.cache->tick();
    EXPECT_FALSE(fixture.cache->isChunkLoaded(20, 20));
}
}  // namespace net::minecraft::test
