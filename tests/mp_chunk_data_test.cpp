#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/world/chunk/MultiplayerChunkCache.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <vector>
namespace {
std::vector<std::uint8_t> makeFullChunkPacketData(int stoneLocalX, int stoneY, int stoneLocalZ, int stoneBlockId) {
  constexpr int sizeX = 16;
  constexpr int sizeY = 128;
  constexpr int sizeZ = 16;
  std::vector<std::uint8_t> data(static_cast<std::size_t>(sizeX * sizeY * sizeZ * 5 / 2), 0);
  int offset = 0;
  for(int localX = 0; localX < sizeX; ++localX) {
    for(int localZ = 0; localZ < sizeZ; ++localZ) {
      for(int y = 0; y < sizeY; ++y) {
        if(localX == stoneLocalX && localZ == stoneLocalZ && y == stoneY) {
          data[static_cast<std::size_t>(offset + y)] = static_cast<std::uint8_t>(stoneBlockId & 0xFF);
        }
      }
      offset += sizeY;
    }
  }
  return data;
}
} // namespace
namespace net::minecraft::test {
TEST(MultiplayerChunkData, FreshClientWorldHasNoLoadedChunks) {
  net::minecraft::client::multiplayer::ClientNetworkHandler handler(nullptr);
  ClientWorld world(&handler, 12345ULL, 0);
  EXPECT_FALSE(world.isPosLoaded(8, 64, 8));
  EXPECT_EQ(world.getChunkIfLoaded(8, 8), nullptr);
}

TEST(MultiplayerChunkData, MapChunkWithoutPreChunkStoresBlocks) {
  net::minecraft::client::multiplayer::ClientNetworkHandler handler(nullptr);
  ClientWorld world(&handler, 12345ULL, 0);
  auto* chunkCache =
      dynamic_cast<net::minecraft::client::world::chunk::MultiplayerChunkCache*>(world.getChunkSource());
  constexpr int chunkX = 0;
  constexpr int chunkZ = 0;
  constexpr int stoneLocalX = 8;
  constexpr int stoneY = 63;
  constexpr int stoneLocalZ = 8;
  ASSERT_NE(chunkCache, nullptr);
  ASSERT_FALSE(chunkCache->hasRealChunk(chunkX, chunkZ));
  const std::vector<std::uint8_t> chunkData =
      makeFullChunkPacketData(stoneLocalX, stoneY, stoneLocalZ, Block::STONE->id);
  world.handleChunkDataUpdate(chunkX * 16, 0, chunkZ * 16, 16, 128, 16, chunkData);
  ASSERT_TRUE(chunkCache->hasRealChunk(chunkX, chunkZ));
  EXPECT_EQ(world.getBlockId(stoneLocalX, stoneY, stoneLocalZ), Block::STONE->id);
}
} // namespace net::minecraft::test
