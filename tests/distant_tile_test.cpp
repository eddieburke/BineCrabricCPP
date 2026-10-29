#include <gtest/gtest.h>
#include <algorithm>
#include <map>
#include <memory>
#include <utility>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/distant/DistantTile.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightType.hpp"

namespace net::minecraft::test {
namespace {
using namespace client::render::distant;

constexpr int kStone = 1;
constexpr int kGrass = 2;
constexpr int kDirt = 3;
constexpr int kWater = 9;

struct ChunkGrid {
 std::map<std::pair<int, int>, std::unique_ptr<Chunk>> chunks;

 Chunk& add(int cx, int cz, int groundTop) {
  auto chunk = std::make_unique<Chunk>(nullptr, cx, cz);
  for(int x = 0; x < 16; ++x) {
   for(int z = 0; z < 16; ++z) {
    for(int y = 0; y < groundTop; ++y) {
     chunk->blocks[(x << 11) | (z << 7) | y] = static_cast<std::uint8_t>(y == groundTop - 1 ? kGrass : kStone);
    }
    chunk->heightmap[(z << 4) | x] = static_cast<std::uint8_t>(groundTop);
   }
  }
  Chunk& ref = *chunk;
  chunks[{cx, cz}] = std::move(chunk);
  return ref;
 }

 void fillTile(int tileX, int tileZ, int groundTop) {
  for(int cz = tileZ * 4 - 1; cz <= tileZ * 4 + 4; ++cz) {
   for(int cx = tileX * 4 - 1; cx <= tileX * 4 + 4; ++cx) {
    add(cx, cz, groundTop);
   }
  }
 }

 [[nodiscard]] ChunkLookup lookup() const {
  return [this](int cx, int cz) -> const Chunk* {
   const auto it = chunks.find({cx, cz});
   return it == chunks.end() ? nullptr : it->second.get();
  };
 }
};

void setColumn(Chunk& chunk, int x, int z, int groundTop, int waterTop) {
 for(int y = 0; y < 128; ++y) {
  std::uint8_t id = 0;
  if(y < groundTop) {
   id = static_cast<std::uint8_t>(y == groundTop - 1 ? kDirt : kStone);
  } else if(y < waterTop) {
   id = kWater;
  }
  chunk.blocks[(x << 11) | (z << 7) | y] = id;
 }
 chunk.heightmap[(z << 4) | x] = static_cast<std::uint8_t>(std::max(groundTop, waterTop));
}

TileCells flatCells(int stride, int top, int block) {
 TileCells cells;
 cells.stride = stride;
 cells.side = kTileBlocks / stride;
 cells.cells.assign(static_cast<std::size_t>((cells.side + 2) * (cells.side + 2)), ColumnCell{});
 for(ColumnCell& cell : cells.cells) {
  cell.present = true;
  cell.runCount = 1;
  cell.runs[0].top = static_cast<std::uint8_t>(top);
  cell.runs[0].bottom = 0;
  cell.runs[0].block = static_cast<std::uint8_t>(block);
  cell.runs[0].topLight = 0xA5;
  cell.light.fill(0xA5);
 }
 return cells;
}
} // namespace


TEST(DistantTile, FlatPlaneWithWaterPatch) {
 ChunkGrid grid;
 grid.fillTile(0, 0, 64);
 setColumn(*grid.chunks[{1, 1}], 0, 0, 60, 64);
 grid.chunks[{2, 2}]->setLight(LightType::Sky, 3, 64, 3, 15);
 grid.chunks[{2, 2}]->setLight(LightType::Block, 3, 64, 3, 7);
 TileCells cells;
 ASSERT_TRUE(extractTile(grid.lookup(), 0, 0, 1, cells));
 EXPECT_EQ(cells.side, 64);
 EXPECT_EQ(cells.cells.size(), 66u * 66u);
 const ColumnCell& flat = cells.at(5, 5);
 EXPECT_TRUE(flat.present);
 EXPECT_EQ(flat.runs[0].top, 64);
 EXPECT_EQ(flat.runs[0].block, kGrass);
 EXPECT_EQ(flat.waterTop, 0);
 const ColumnCell& wet = cells.at(16, 16);
 EXPECT_EQ(wet.runs[0].top, 60);
 EXPECT_EQ(wet.runs[0].block, kDirt);
 EXPECT_EQ(wet.waterTop, 64);
 EXPECT_EQ(cells.at(32 + 3, 32 + 3).runs[0].topLight, 0x7F);
 EXPECT_EQ(cells.at(32 + 3, 32 + 3).light[64], 0x7F);
 EXPECT_EQ(cells.at(-1, -1).runs[0].top, 64);
 EXPECT_TRUE(cells.at(64, 64).present);
}

TEST(DistantTile, StrideTakesMaxColumn) {
 ChunkGrid grid;
 grid.fillTile(0, 0, 64);
 setColumn(*grid.chunks[{0, 0}], 5, 6, 71, 0);
 TileCells cells;
 ASSERT_TRUE(extractTile(grid.lookup(), 0, 0, 4, cells));
 EXPECT_EQ(cells.side, 16);
 EXPECT_EQ(cells.cells.size(), 18u * 18u);
 const ColumnCell& raised = cells.at(1, 1);
 EXPECT_EQ(raised.runs[0].top, 71);
 EXPECT_EQ(raised.runs[0].block, kDirt);
 EXPECT_EQ(cells.at(0, 0).runs[0].top, 64);
 EXPECT_EQ(cells.at(2, 1).runs[0].top, 64);
}

TEST(DistantTile, MissingBorderIsNotPresentAndMissingOwnFails) {
 ChunkGrid grid;
 grid.fillTile(0, 0, 64);
 grid.chunks.erase({-1, 2});
 TileCells cells;
 ASSERT_TRUE(extractTile(grid.lookup(), 0, 0, 1, cells));
 EXPECT_FALSE(cells.at(-1, 40).present);
 EXPECT_TRUE(cells.at(-1, 10).present);
 EXPECT_TRUE(cells.at(0, 40).present);
 grid.chunks.erase({1, 1});
 EXPECT_FALSE(extractTile(grid.lookup(), 0, 0, 1, cells));
}

TEST(DistantTile, CanopyOverLitGroundKeepsTwoRuns) {
 ChunkGrid grid;
 grid.fillTile(0, 0, 64);
 Chunk& chunk = *grid.chunks[{1, 1}];
 for(int y = 70; y < 73; ++y) chunk.blocks[(4 << 11) | (4 << 7) | y] = 18;
 chunk.heightmap[(4 << 4) | 4] = 73;
 for(int y = 64; y < 70; ++y) chunk.setLight(LightType::Sky, 4, y, 4, 12);
 TileCells cells;
 ASSERT_TRUE(extractTile(grid.lookup(), 0, 0, 1, cells));
 const ColumnCell& tree = cells.at(20, 20);
 ASSERT_EQ(tree.runCount, 2);
 EXPECT_EQ(tree.runs[0].top, 73);
 EXPECT_EQ(tree.runs[0].bottom, 70);
 EXPECT_EQ(tree.runs[0].block, 18);
 EXPECT_EQ(tree.runs[1].top, 64);
 EXPECT_EQ(tree.runs[1].block, kGrass);
 TileMesh mesh;
 meshTile(cells, defaultPalette(), mesh);
 std::size_t undersides = 0;
 for(std::size_t i = 0; i < mesh.opaque.size(); i += 4) {
  if(mesh.opaque[i].normal == DhNormal::NegY) ++undersides;
 }
 EXPECT_EQ(undersides, 1u);
}

TEST(DistantTile, SealedCaveFillsIntoOneRun) {
 ChunkGrid grid;
 grid.fillTile(0, 0, 64);
 Chunk& chunk = *grid.chunks[{1, 1}];
 for(int y = 30; y < 40; ++y) chunk.blocks[(4 << 11) | (4 << 7) | y] = 0;
 TileCells cells;
 ASSERT_TRUE(extractTile(grid.lookup(), 0, 0, 1, cells));
 const ColumnCell& cell = cells.at(20, 20);
 ASSERT_EQ(cell.runCount, 1);
 EXPECT_EQ(cell.runs[0].top, 64);
 EXPECT_EQ(cell.runs[0].bottom, 0);
}

TEST(DistantTile, PaletteMaterials) {
 const BlockPalette palette = defaultPalette();
 EXPECT_EQ(palette.entries[0].material, DhMaterial::Air);
 EXPECT_EQ(palette.entries[1].material, DhMaterial::Stone);
 EXPECT_EQ(palette.entries[2].material, DhMaterial::Grass);
 EXPECT_EQ(palette.entries[9].material, DhMaterial::Water);
 EXPECT_TRUE(palette.entries[9].translucent);
 EXPECT_EQ(palette.entries[9].top[3], 146);
 EXPECT_EQ(palette.entries[12].material, DhMaterial::Sand);
 EXPECT_EQ(palette.entries[87].material, DhMaterial::NetherStone);
 EXPECT_EQ(palette.entries[89].material, DhMaterial::Illuminated);
}

TEST(DistantMesher, FlatTileMergesToOneQuadPerRow) {
 const TileCells cells = flatCells(1, 64, kStone);
 TileMesh mesh;
 meshTile(cells, defaultPalette(), mesh);
 EXPECT_EQ(mesh.opaque.size(), 64u * 4u);
 EXPECT_TRUE(mesh.water.empty());
 for(const DhVertex& vertex : mesh.opaque) {
  EXPECT_EQ(vertex.normal, DhNormal::PosY);
  EXPECT_EQ(vertex.y, 64);
 }
 EXPECT_EQ(mesh.minY, 64);
 EXPECT_EQ(mesh.maxY, 64);
}

TEST(DistantMesher, RaisedColumnGetsFourSides) {
 TileCells cells = flatCells(1, 64, kStone);
 cells.at(10, 10).runs[0].top = 70;
 TileMesh mesh;
 meshTile(cells, defaultPalette(), mesh);
 std::size_t tops = 0;
 std::size_t sides = 0;
 for(std::size_t i = 0; i < mesh.opaque.size(); i += 4) {
  const DhVertex& v = mesh.opaque[i];
  if(v.normal == DhNormal::PosY) {
   ++tops;
  } else {
   ++sides;
   int lo = 255;
   int hi = 0;
   for(int k = 0; k < 4; ++k) {
    lo = std::min<int>(lo, mesh.opaque[i + k].y);
    hi = std::max<int>(hi, mesh.opaque[i + k].y);
   }
   EXPECT_EQ(lo, 64);
   EXPECT_EQ(hi, 70);
  }
 }
 EXPECT_EQ(tops, 64u + 2u);
 EXPECT_EQ(sides, 4u);
 EXPECT_EQ(mesh.minY, 64);
 EXPECT_EQ(mesh.maxY, 70);
}

TEST(DistantMesher, MissingNeighbourGetsSkirt) {
 TileCells cells = flatCells(1, 64, kStone);
 cells.at(-1, 5).present = false;
 TileMesh mesh;
 meshTile(cells, defaultPalette(), mesh);
 EXPECT_EQ(mesh.opaque.size(), (64u + 1u) * 4u);
 std::size_t skirts = 0;
 for(std::size_t i = 0; i < mesh.opaque.size(); i += 4) {
  if(mesh.opaque[i].normal == DhNormal::NegX) {
   ++skirts;
   int lo = 255;
   for(int k = 0; k < 4; ++k) {
    lo = std::min<int>(lo, mesh.opaque[i + k].y);
    EXPECT_EQ(mesh.opaque[i + k].x, 0);
   }
   EXPECT_EQ(lo, 0);
  }
 }
 EXPECT_EQ(skirts, 1u);
 EXPECT_EQ(mesh.minY, 0);
}
TEST(DistantMesher, CliffLightFollowsAdjacentHeightInsteadOfNeighborTop) {
 TileCells cells = flatCells(1, 64, kStone);
 cells.at(10, 10).runs[0].top = 70;
 auto& neighbor = cells.at(9, 10);
 neighbor.runs[0].topLight = 0;
 neighbor.light.fill(0x0F);
 neighbor.light[64] = 0;
 neighbor.light[65] = 0;
 TileMesh mesh;
 meshTile(cells, defaultPalette(), mesh);
 int litArea = 0;
 int shadedArea = 0;
 for(std::size_t i = 0; i < mesh.opaque.size(); i += 4) {
  const auto* quad = &mesh.opaque[i];
  if(quad[0].normal != DhNormal::NegX || quad[0].x != 10 ||
     std::min({quad[0].z, quad[1].z, quad[2].z, quad[3].z}) != 10) continue;
  int bottom = 128;
  int top = 0;
  for(int v = 0; v < 4; ++v) {
   bottom = std::min<int>(bottom, quad[v].y);
   top = std::max<int>(top, quad[v].y);
  }
  for(int v = 0; v < 4; ++v) EXPECT_EQ(quad[v].meta & 0xFF, bottom < 66 ? 0 : 0x0F);
  if(bottom < 66) shadedArea += top - bottom;
  else litArea += top - bottom;
 }
 EXPECT_EQ(shadedArea, 2);
 EXPECT_EQ(litArea, 4);
}

TEST(DistantMesher, VertexLayoutAndWater) {
 TileCells cells = flatCells(1, 64, kStone);
 cells.at(10, 10).runs[0].top = 70;
 cells.at(20, 20).waterTop = 66;
 cells.at(20, 20).runs[0].top = 60;
 TileMesh mesh;
 meshTile(cells, defaultPalette(), mesh);
 bool sawNegZ = false;
 bool sawPosZ = false;
 bool sawNegX = false;
 bool sawPosX = false;
 for(const DhVertex& v : mesh.opaque) {
  EXPECT_EQ(v.meta & 0xFF, 0xA5);
  EXPECT_EQ(v.meta >> 8, 0);
  EXPECT_EQ(v.material, DhMaterial::Stone);
  EXPECT_EQ(v.color[3], 255);
  switch(v.normal) {
   case DhNormal::PosY:
    EXPECT_EQ(v.color[0], 125);
    break;
   case DhNormal::NegZ:
    sawNegZ = true;
    EXPECT_EQ(v.color[0], 100);
    break;
   case DhNormal::PosZ:
    sawPosZ = true;
    EXPECT_EQ(v.color[0], 100);
    break;
   case DhNormal::NegX:
    sawNegX = true;
    EXPECT_EQ(v.color[0], 75);
    break;
   case DhNormal::PosX:
    sawPosX = true;
    EXPECT_EQ(v.color[0], 75);
    break;
   default:
    ADD_FAILURE() << "unexpected normal " << static_cast<int>(v.normal);
  }
 }
 EXPECT_TRUE(sawNegZ && sawPosZ && sawNegX && sawPosX);
 ASSERT_EQ(mesh.water.size(), 4u);
 for(const DhVertex& v : mesh.water) {
  EXPECT_EQ(v.normal, DhNormal::PosY);
  EXPECT_EQ(v.material, DhMaterial::Water);
  EXPECT_EQ(v.color[3], 146);
  EXPECT_EQ(v.y, 66);
 }
}

TEST(DistantMesher, WindingIsCounterClockwiseFromOutside) {
 TileCells cells = flatCells(1, 64, kStone);
 cells.at(10, 10).runs[0].top = 70;
 TileMesh mesh;
 meshTile(cells, defaultPalette(), mesh);
 const float expected[6][3] = {{0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};
 for(std::size_t i = 0; i < mesh.opaque.size(); i += 4) {
  const DhVertex* q = &mesh.opaque[i];
  const float a[3] = {float(q[1].x) - q[0].x, float(q[1].y) - q[0].y, float(q[1].z) - q[0].z};
  const float b[3] = {float(q[2].x) - q[0].x, float(q[2].y) - q[0].y, float(q[2].z) - q[0].z};
  const float n[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
  const float* e = expected[q[0].normal];
  EXPECT_GT(n[0] * e[0] + n[1] * e[1] + n[2] * e[2], 0.0f);
 }
}

class DistantSlabTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
   if(glfwInit() != GLFW_TRUE) {
    return;
   }
   glfwDefaultWindowHints();
   glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
   window_ = glfwCreateWindow(64, 64, "distant-slab-test", nullptr, nullptr);
   if(window_ != nullptr) {
    glfwMakeContextCurrent(window_);
    client::gl::GLCore::ensureLoaded();
   }
  }
  static void TearDownTestSuite() {
   if(window_ != nullptr) {
    client::render::core::releaseGlResources();
    glfwMakeContextCurrent(nullptr);
    glfwDestroyWindow(window_);
    window_ = nullptr;
   }
  }
  void SetUp() override {
   if(window_ == nullptr) {
    GTEST_SKIP() << "no GL context";
   }
   glfwMakeContextCurrent(window_);
  }
  static GLFWwindow* window_;
};
GLFWwindow* DistantSlabTest::window_ = nullptr;

constexpr std::size_t kPageBytes = DistantSlab::kPageVertices * sizeof(DhVertex);

TEST_F(DistantSlabTest, CreateRoundsToPagesAndBuildsVao) {
 DistantSlab slab;
 ASSERT_TRUE(slab.create(kPageBytes * 3 + 1));
 EXPECT_EQ(slab.capacityBytes(), kPageBytes * 4);
 EXPECT_EQ(slab.usedBytes(), 0u);
 EXPECT_NE(slab.vao(), 0u);
}

TEST_F(DistantSlabTest, FullSlabReturnsNulloptAndNoGrowth) {
 DistantSlab slab;
 ASSERT_TRUE(slab.create(kPageBytes * 4));
 const auto a = slab.allocate(DistantSlab::kPageVertices * 3);
 ASSERT_TRUE(a.has_value());
 EXPECT_EQ(a->pageCount, 3u);
 EXPECT_EQ(a->baseVertex(), 0u);
 EXPECT_FALSE(slab.allocate(DistantSlab::kPageVertices * 2).has_value());
 const auto b = slab.allocate(1);
 ASSERT_TRUE(b.has_value());
 EXPECT_EQ(b->firstPage, 3u);
 EXPECT_FALSE(slab.allocate(1).has_value());
 EXPECT_EQ(slab.usedBytes(), kPageBytes * 4);
 EXPECT_EQ(slab.capacityBytes(), kPageBytes * 4);
}

TEST_F(DistantSlabTest, ReleaseCoalescesNeighbours) {
 DistantSlab slab;
 ASSERT_TRUE(slab.create(kPageBytes * 6));
 const auto a = slab.allocate(DistantSlab::kPageVertices * 2);
 const auto b = slab.allocate(DistantSlab::kPageVertices * 2);
 const auto c = slab.allocate(DistantSlab::kPageVertices * 2);
 ASSERT_TRUE(a && b && c);
 EXPECT_FALSE(slab.allocate(1).has_value());
 slab.release(*a);
 slab.release(*c);
 EXPECT_FALSE(slab.allocate(DistantSlab::kPageVertices * 3).has_value());
 slab.release(*b);
 EXPECT_EQ(slab.usedBytes(), 0u);
 const auto whole = slab.allocate(DistantSlab::kPageVertices * 6);
 ASSERT_TRUE(whole.has_value());
 EXPECT_EQ(whole->firstPage, 0u);
}

TEST_F(DistantSlabTest, UploadWritesWithoutGlError) {
 DistantSlab slab;
 ASSERT_TRUE(slab.create(kPageBytes * 2));
 const auto a = slab.allocate(8);
 ASSERT_TRUE(a.has_value());
 std::vector<DhVertex> vertices(8);
 slab.upload(*a, vertices);
 slab.release(*a);
 slab.destroy();
 EXPECT_EQ(slab.capacityBytes(), 0u);
}
} // namespace net::minecraft::test
