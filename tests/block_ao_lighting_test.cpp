#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <cstdint>
#include <memory>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
namespace net::minecraft::test {
namespace {
using client::render::Tessellator;
using client::render::TessellatorMesh;
using client::render::TessellatorVertex;
using client::render::chunk::RegionSnapshot;
constexpr int kSectionSize = 16;
std::array<float, 16> linearLuminance() {
 std::array<float, 16> table{};
 for(int level = 0; level < 16; ++level)
  table[static_cast<std::size_t>(level)] = Dimension::luminanceForLightLevel(level);
 return table;
}
std::size_t blockIndex(int x, int y, int z) {
 return static_cast<std::size_t>((x << 11) | (z << 7) | y);
}
struct MeshedBlock {
 std::unique_ptr<Chunk> chunk;
 std::unique_ptr<RegionSnapshot> snapshot;
 TessellatorMesh mesh;
};
MeshedBlock meshBlock(std::unique_ptr<Chunk> chunk, block::Block& block, int x, int y, int z,
                      float aoStrength = 1.0f) {
 MeshedBlock result;
 result.chunk = std::move(chunk);
 result.chunk->populateHeightMapOnly();
 std::vector<RegionSnapshot::SourceChunk> sources{RegionSnapshot::SourceChunk{0, 0, result.chunk.get()}};
 result.snapshot = std::make_unique<RegionSnapshot>(sources, 0, linearLuminance(), nullptr, -1, -1, -1,
                                                    kSectionSize + 1, kSectionSize, kSectionSize + 1);
 client::option::RenderSettings settings{};
 settings.ambientOcclusionActive = true;
 settings.ambientOcclusionStrength = aoStrength;
 Tessellator tessellator;
 tessellator.setCaptureOnly(true);
 client::render::block::BlockRenderManager manager(tessellator, result.snapshot.get(), settings);
 tessellator.startQuads();
 manager.render(block, x, y, z);
 result.mesh = tessellator.takeMesh();
 return result;
}
int blockLight(const TessellatorVertex& vertex) {
 return static_cast<int>(static_cast<std::uint32_t>(vertex.light) & 0xFFFFu);
}
int skyLight(const TessellatorVertex& vertex) {
 return static_cast<int>((static_cast<std::uint32_t>(vertex.light) >> 16U) & 0xFFFFu);
}
float normalY(const TessellatorVertex& vertex) {
 return static_cast<float>(static_cast<std::int8_t>((vertex.normal >> 8U) & 0xFF)) / 127.0f;
}
}
TEST(BlockAoLighting, ZeroCornerLightFallsBackToFaceCenter) {
 constexpr int x = 8;
 constexpr int y = 8;
 constexpr int z = 8;
 auto chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 chunk->blocks[blockIndex(x, y, z)] = static_cast<std::uint8_t>(block::Block::STONE->id);
 chunk->blockLight.set(x, y + 1, z, 12);
 chunk->skyLight.set(x, y + 1, z, 10);
 const MeshedBlock scene = meshBlock(std::move(chunk), *block::Block::STONE, x, y, z);
 bool found = false;
 for(const auto& vertex : scene.mesh.vertices) {
  if(std::fabs(vertex.x - 9.0f) > 0.01f || std::fabs(vertex.y - 9.0f) > 0.01f ||
     std::fabs(vertex.z - 9.0f) > 0.01f || normalY(vertex) < 0.9f) {
   continue;
  }
  found = true;
  EXPECT_EQ(blockLight(vertex), 12 * 16);
  EXPECT_EQ(skyLight(vertex), 10 * 16);
 }
 EXPECT_TRUE(found);
}
TEST(BlockAoLighting, TorchLitCellWithoutSkyDoesNotBorrowCenterSky) {
 constexpr int x = 8;
 constexpr int y = 8;
 constexpr int z = 8;
 auto chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 chunk->blocks[blockIndex(x, y, z)] = static_cast<std::uint8_t>(block::Block::STONE->id);
 for(int sampleX = x - 1; sampleX <= x + 1; ++sampleX) {
  for(int sampleZ = z - 1; sampleZ <= z + 1; ++sampleZ) {
   chunk->blockLight.set(sampleX, y + 1, sampleZ, 12);
   chunk->skyLight.set(sampleX, y + 1, sampleZ, 0);
  }
 }
 chunk->blockLight.set(x, y + 1, z, 0);
 chunk->skyLight.set(x, y + 1, z, 12);
 const MeshedBlock scene = meshBlock(std::move(chunk), *block::Block::STONE, x, y, z);
 bool found = false;
 for(const auto& vertex : scene.mesh.vertices) {
  if(std::fabs(vertex.x - 9.0f) > 0.01f || std::fabs(vertex.y - 9.0f) > 0.01f ||
     std::fabs(vertex.z - 9.0f) > 0.01f || normalY(vertex) < 0.9f) {
   continue;
  }
  found = true;
  EXPECT_EQ(skyLight(vertex), 3 * 16);
  EXPECT_EQ(blockLight(vertex), 9 * 16);
 }
 EXPECT_TRUE(found);
}
TEST(BlockAoLighting, InsetStairTopSamplesItsOwnLightPlane) {
 constexpr int x = 8;
 constexpr int y = 8;
 constexpr int z = 8;
 auto chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 chunk->blocks[blockIndex(x, y, z)] = static_cast<std::uint8_t>(block::Block::COBBLESTONE_STAIRS->id);
 chunk->meta.set(x, y, z, 2);
 for(int sampleX = x - 1; sampleX <= x + 1; ++sampleX) {
  for(int sampleZ = z - 1; sampleZ <= z + 1; ++sampleZ) {
   chunk->blockLight.set(sampleX, y, sampleZ, 12);
   chunk->skyLight.set(sampleX, y, sampleZ, 10);
  }
 }
 chunk->blockLight.set(x, y, z, 0);
 chunk->skyLight.set(x, y, z, 0);
 const MeshedBlock scene = meshBlock(std::move(chunk), *block::Block::COBBLESTONE_STAIRS, x, y, z);
 int vertices = 0;
 for(const auto& vertex : scene.mesh.vertices) {
  if(std::fabs(vertex.y - 8.5f) > 0.01f || vertex.z > 8.51f || normalY(vertex) < 0.9f) continue;
  ++vertices;
  EXPECT_EQ(blockLight(vertex), 12 * 16);
  EXPECT_EQ(skyLight(vertex), 10 * 16);
 }
 EXPECT_EQ(vertices, 4);
}

// see third_party/iris/common/src/main/java/net/irisshaders/iris/mixin/MixinBlockStateBehavior.java:44
// ambientOcclusionLevel reaches Java only through getShadeBrightness, which feeds the
// occlusion coefficient. Folding it into the lightmap too collapsed smooth lighting
// toward the flat face value and drew a per-block grid wherever the light varied.
TEST(BlockAoLighting, AoStrengthDoesNotFlattenTheLightmap) {
 constexpr int x = 8;
 constexpr int y = 8;
 constexpr int z = 8;
 const auto build = [] {
  auto chunk = std::make_unique<Chunk>(nullptr, 0, 0);
  chunk->blocks[blockIndex(x, y, z)] = static_cast<std::uint8_t>(block::Block::STONE->id);
  for(int sampleX = x - 1; sampleX <= x + 1; ++sampleX) {
   for(int sampleZ = z - 1; sampleZ <= z + 1; ++sampleZ) {
    chunk->blockLight.set(sampleX, y + 1, sampleZ, 15);
    chunk->skyLight.set(sampleX, y + 1, sampleZ, 15);
   }
  }
  // One low corner, so the four top-face corners cannot all average the same.
  // Non-zero: a cell whose packed light is zero reads as the face centre.
  chunk->blockLight.set(x + 1, y + 1, z + 1, 3);
  chunk->skyLight.set(x + 1, y + 1, z + 1, 3);
  return chunk;
 };
 const auto topFaceLight = [](const MeshedBlock& scene) {
  std::vector<std::pair<int, int>> light;
  for(const auto& vertex : scene.mesh.vertices) {
   if(std::fabs(vertex.y - 9.0f) > 0.01f || normalY(vertex) < 0.9f) continue;
   light.emplace_back(blockLight(vertex), skyLight(vertex));
  }
  std::sort(light.begin(), light.end());
  return light;
 };
 const std::vector<std::pair<int, int>> full = topFaceLight(meshBlock(build(), *block::Block::STONE, x, y, z, 1.0f));
 const std::vector<std::pair<int, int>> half = topFaceLight(meshBlock(build(), *block::Block::STONE, x, y, z, 0.5f));
 ASSERT_FALSE(full.empty());
 EXPECT_NE(full.front(), full.back()) << "the fixture has to produce corners that actually differ";
 EXPECT_EQ(half, full) << "ambientOcclusionLevel pulled the lightmap toward the flat face value";
}

}
