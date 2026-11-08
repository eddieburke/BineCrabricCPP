#include <gtest/gtest.h>
#include <algorithm>
#include <map>
#include <memory>
#include "net/minecraft/client/render/distant/NetherTile.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightType.hpp"

namespace net::minecraft::test {
namespace {
using namespace client::render::distant;
struct NetherGrid {
 std::map<std::pair<int, int>, std::unique_ptr<Chunk>> chunks;
 void fill(int tileX, int tileZ, bool layered = false) {
  for(int z = tileZ * kTileChunks - 1; z <= tileZ * kTileChunks + kTileChunks; ++z) {
   for(int x = tileX * kTileChunks - 1; x <= tileX * kTileChunks + kTileChunks; ++x) {
    auto chunk = std::make_unique<Chunk>(nullptr, x, z);
    for(int cz = 0; cz < 16; ++cz) {
     for(int cx = 0; cx < 16; ++cx) {
      chunk->heightmap[(cz << 4) | cx] = 128;
      for(int y = 0; y < kNetherHeight; ++y) {
       const int id = layered ? (y / 8 % 2 == 0 ? 87 : 0) : y < 30 || y >= 96 ? 87 : y < 32 ? 11 : 0;
       chunk->blocks[(cx << 11) | (cz << 7) | y] = static_cast<std::uint8_t>(id);
       chunk->setLight(LightType::Block, cx, y, cz, 8);
       chunk->setLight(LightType::Sky, cx, y, cz, 15);
      }
     }
    }
    chunks[{x, z}] = std::move(chunk);
   }
  }
 }
 ChunkLookup lookup() const {
  return [this](int x, int z) -> const Chunk* {
   const auto found = chunks.find({x, z});
   return found == chunks.end() ? nullptr : found->second.get();
  };
 }
};
}

TEST(NetherLod, PreservesEveryDarkCaveLayerAtEveryStride) {
 NetherGrid grid;
 grid.fill(0, 0, true);
 for(int stride : {1, 2, 4, 8, 16}) {
  NetherTileCells cells;
  ASSERT_TRUE(extractNetherTile(grid.lookup(), 0, 0, stride, cells));
  EXPECT_EQ(cells.chunkMask, 0xFFFFu);
  EXPECT_EQ(cells.columns.size(), static_cast<std::size_t>((64 / stride + 2) * (64 / stride + 2)));
  for(const auto& column : cells.columns) {
   ASSERT_TRUE(column.present);
   for(int y = 0; y < kNetherHeight; ++y) {
    EXPECT_EQ(column.blocks[y], y / 8 % 2 == 0 ? 87 : 0);
    EXPECT_EQ(column.light[y], 0x80);
   }
  }
 }
}

TEST(NetherLod, GreedyMeshKeepsLavaFloorAndDownwardCeiling) {
 NetherGrid grid;
 grid.fill(0, 0);
 const auto palette = defaultPalette();
 constexpr int normals[6][3] = {{0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};
 for(int stride : {1, 4, 16}) {
  NetherTileCells cells;
  ASSERT_TRUE(extractNetherTile(grid.lookup(), 0, 0, stride, cells));
  TileMesh mesh;
  meshNetherTile(cells, palette, mesh);
  EXPECT_TRUE(mesh.water.empty());
  EXPECT_LE(mesh.opaque.size(), 24u * 4u);
  EXPECT_EQ(mesh.minY, 0);
  EXPECT_EQ(mesh.maxY, 128);
  int floorFaces = 0;
  int ceilingFaces = 0;
  for(std::size_t i = 0; i < mesh.opaque.size(); i += 4) {
   const auto* quad = &mesh.opaque[i];
   const int ax = int(quad[1].x) - quad[0].x;
   const int ay = int(quad[1].y) - quad[0].y;
   const int az = int(quad[1].z) - quad[0].z;
   const int bx = int(quad[2].x) - quad[0].x;
   const int by = int(quad[2].y) - quad[0].y;
   const int bz = int(quad[2].z) - quad[0].z;
   const auto* normal = normals[quad->normal];
   EXPECT_GT((ay * bz - az * by) * normal[0] + (az * bx - ax * bz) * normal[1] +
             (ax * by - ay * bx) * normal[2], 0);
   if(quad->normal == DhNormal::PosY && quad->y == 32) {
    ++floorFaces;
    EXPECT_EQ(quad->material, DhMaterial::Lava);
    EXPECT_EQ(quad->meta, 0xF0);
   }
   if(quad->normal == DhNormal::NegY && quad->y == 96) {
    ++ceilingFaces;
    EXPECT_EQ(quad->material, DhMaterial::NetherStone);
    EXPECT_EQ(quad->meta, 0x80);
   }
  }
  EXPECT_EQ(floorFaces, 1);
  EXPECT_EQ(ceilingFaces, 1);
 }
}

TEST(NetherLod, NegativeCoordinatesAndPartialCoverageDoNotCoverMissingChunks) {
 NetherGrid grid;
 grid.fill(-1, -1);
 grid.chunks.erase({-3, -3});
 NetherTileCells cells;
 EXPECT_FALSE(extractNetherTile(grid.lookup(), -1, -1, 4, cells));
 ASSERT_TRUE(extractNetherTile(grid.lookup(), -1, -1, 4, cells, true));
 EXPECT_EQ(cells.chunkMask, 0xFFFFu & ~(1u << 5));
 EXPECT_FALSE(cells.at(4, 4).present);
 EXPECT_TRUE(cells.at(0, 0).present);
 TileMesh mesh;
 meshNetherTile(cells, defaultPalette(), mesh);
 ASSERT_FALSE(mesh.opaque.empty());
 for(const auto& vertex : mesh.opaque) {
  EXPECT_LE(vertex.x, 64);
  EXPECT_LE(vertex.z, 64);
  EXPECT_LE(vertex.y, 128);
 }
}


TEST(NetherLod, GlowstoneInsideDarkCaveKeepsItsEmission) {
 NetherGrid grid;
 grid.fill(0, 0);
 auto& chunk = *grid.chunks[{1, 1}];
 chunk.blocks[(4 << 11) | (4 << 7) | 60] = 89;
 for(int y = 59; y <= 61; ++y) chunk.setLight(LightType::Block, 4, y, 4, 0);
 NetherTileCells cells;
 ASSERT_TRUE(extractNetherTile(grid.lookup(), 0, 0, 1, cells));
 TileMesh mesh;
 meshNetherTile(cells, defaultPalette(), mesh);
 int glowstoneFaces = 0;
 for(std::size_t index = 0; index < mesh.opaque.size(); index += 4) {
  if(mesh.opaque[index].material != DhMaterial::Illuminated) continue;
  ++glowstoneFaces;
  EXPECT_EQ(mesh.opaque[index].meta, 0xF0);
 }
 EXPECT_EQ(glowstoneFaces, 6);
}
}
