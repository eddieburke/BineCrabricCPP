#include "support/server_test_macros.hpp"
#include "net/minecraft/server/world/chunk/ServerChunkCache.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"
#include <iostream>
#include <limits>
#include <string>
namespace {
struct ServerChunkCacheFixture {
  net::minecraft::server::MinecraftServer server;
  net::minecraft::EmptyWorldStorage storage;
  net::minecraft::ServerWorld world{&server, &storage, "test", 0, 1};
  net::minecraft::server::world::chunk::ServerChunkCache* cache = nullptr;
  ServerChunkCacheFixture() {
    world.setSpawnPos(net::minecraft::Vec3i{0, 64, 0});
    cache = world.chunkCache;
    EXPECT_TRUE(cache != nullptr);
  }
};
void testChunkPosHashMatchesJava() {
  EXPECT_EQ(net::minecraft::chunkPosHashCode(0, 0), 0);
  EXPECT_EQ(net::minecraft::chunkPosHashCode(1, 1), (1 << 16) | 1);
  EXPECT_EQ(net::minecraft::chunkPosHashCode(-1, -1),
            std::numeric_limits<int>::min() | (32767 << 16) | 32768 | 32767);
  EXPECT_EQ(net::minecraft::chunkPosHashCode(32767, 32767), (32767 << 16) | 32767);
  EXPECT_EQ(net::minecraft::chunkPosHashCode(-32768, 0), std::numeric_limits<int>::min());
}
void testIsLoadedUsesSpawnRadius() {
  ServerChunkCacheFixture fixture;
  fixture.cache->forceLoad = true;
  fixture.cache->loadChunk(5, 5);
  EXPECT_TRUE(fixture.cache->isChunkLoaded(5, 5));
  fixture.cache->isLoaded(5, 5);
  EXPECT_TRUE(fixture.cache->getDebugInfo().find("Drop: 0") != std::string::npos);
  fixture.cache->loadChunk(12, 12);
  EXPECT_TRUE(fixture.cache->isChunkLoaded(12, 12));
  fixture.cache->isLoaded(12, 12);
  EXPECT_TRUE(fixture.cache->getDebugInfo().find("Drop: 1") != std::string::npos);
}
void testGetChunkReturnsEmptyWhenEventsDisabled() {
  ServerChunkCacheFixture fixture;
  fixture.world.setEventProcessingEnabled(false);
  fixture.cache->forceLoad = false;
  net::minecraft::Chunk& chunk = fixture.cache->getChunk(3, 3);
  EXPECT_TRUE(chunk.empty);
  EXPECT_FALSE(fixture.cache->isChunkLoaded(3, 3));
}
void testForceLoadBypassesEventGate() {
  ServerChunkCacheFixture fixture;
  fixture.world.setEventProcessingEnabled(false);
  fixture.cache->forceLoad = true;
  net::minecraft::Chunk& chunk = fixture.cache->loadChunk(4, 4);
  EXPECT_FALSE(chunk.empty);
  EXPECT_TRUE(fixture.cache->isChunkLoaded(4, 4));
}
void testCanSaveRespectsSavingDisabled() {
  ServerChunkCacheFixture fixture;
  EXPECT_TRUE(fixture.cache->canSave());
  fixture.world.savingDisabled = true;
  EXPECT_FALSE(fixture.cache->canSave());
}
void testTickUnloadsMarkedChunks() {
  ServerChunkCacheFixture fixture;
  fixture.cache->forceLoad = true;
  fixture.cache->loadChunk(20, 20);
  EXPECT_TRUE(fixture.cache->isChunkLoaded(20, 20));
  fixture.cache->isLoaded(20, 20);
  fixture.cache->tick();
  EXPECT_FALSE(fixture.cache->isChunkLoaded(20, 20));
}
} // namespace
int main() {
  RUN_SERVER_TEST(testChunkPosHashMatchesJava);
  RUN_SERVER_TEST(testIsLoadedUsesSpawnRadius);
  RUN_SERVER_TEST(testGetChunkReturnsEmptyWhenEventsDisabled);
  RUN_SERVER_TEST(testForceLoadBypassesEventGate);
  RUN_SERVER_TEST(testCanSaveRespectsSavingDisabled);
  RUN_SERVER_TEST(testTickUnloadsMarkedChunks);
  if(server_test::failureCount() != 0) {
    std::cout << server_test::failureCount() << " test(s) failed\n";
    return 1;
  }
  std::cout << "All server chunk cache tests passed\n";
  return 0;
}
