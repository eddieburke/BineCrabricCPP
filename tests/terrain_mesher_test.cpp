#include <gtest/gtest.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "net/minecraft/client/render/terrain/TerrainMesher.hpp"
#include "net/minecraft/client/render/terrain/TerrainSurfaceData.hpp"
namespace {
using namespace net::minecraft::client::render::terrain;
std::vector<std::uint8_t> oneFlatChunk() {
  constexpr int span = kTerrainRegionChunks + 2;
  std::vector<std::uint8_t> snapshot(static_cast<std::size_t>(span * span * kTerrainChunkBytes));
  const std::size_t chunkBase = static_cast<std::size_t>(span + 1) * kTerrainChunkBytes;
  snapshot[chunkBase] = 1;
  for(int column = 0; column < 256; ++column) {
    const std::size_t offset = chunkBase + 1 + static_cast<std::size_t>(column * kTerrainColumnBytes);
    snapshot[offset] = 64;
    snapshot[offset + 1] = 2;
    snapshot[offset + 2] = 3;
  }
  return snapshot;
}
TEST(TerrainMesher, BuildsFlatChunkAtFullDetail) {
  std::array<std::uint32_t, 256> top{};
  std::array<std::uint32_t, 256> side{};
  top[2] = 0xFF332211U;
  side[3] = 0xFF665544U;
  TerrainMeshResult result;
  buildTerrainMesh(oneFlatChunk(), 0, top, side, result);
  EXPECT_EQ(result.vertices.size(), 256U * 6U);
  EXPECT_FLOAT_EQ(result.minY, 65.0f);
  EXPECT_FLOAT_EQ(result.maxY, 65.0f);
  ASSERT_FALSE(result.vertices.empty());
  EXPECT_EQ(result.vertices.front().color, top[2]);
}
TEST(TerrainMesher, CoarsensFlatChunk) {
  std::array<std::uint32_t, 256> top{};
  std::array<std::uint32_t, 256> side{};
  top[2] = 0xFFFFFFFFU;
  side[3] = 0xFFFFFFFFU;
  TerrainMeshResult result;
  buildTerrainMesh(oneFlatChunk(), 4, top, side, result);
  EXPECT_EQ(result.vertices.size(), 6U);
  EXPECT_FLOAT_EQ(result.minY, 65.0f);
  EXPECT_FLOAT_EQ(result.maxY, 65.0f);
}
}
