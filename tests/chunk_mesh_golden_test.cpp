// Golden coverage for the chunk-meshing hot path.
//
// This is the safety net named in docs/singleplayer-render-refactor-handoff.md.
// It pins the geometry the mesher emits for a fixed block/light pattern so the
// planned RegionSnapshot rewrite (flat 18^3 shell) and brightness cache can be
// refactored without silently changing lighting or face culling.
//
// It deliberately drives RegionSnapshot + BlockRenderManager directly rather
// than ChunkBuilder::buildMesh: ChunkMeshJob's factory needs a live World and
// ChunkSource, while everything the refactors touch (snapshot block/light
// accessors, AO corner averaging, face visibility, vertex emission) sits below
// that. No GL context is required — the Tessellator runs capture-only.
//
// If a hash below changes, the RENDERED IMAGE CHANGED. Do not re-bless it
// without confirming the new output is correct in-game.
#include <gtest/gtest.h>
#include <cstdint>
#include <cmath>
#include <memory>
#include <span>
#include <unordered_set>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::client::render::Tessellator;
using net::minecraft::client::render::TessellatorMesh;
using net::minecraft::client::render::chunk::RegionSnapshot;
constexpr int kSectionSize = 16;
// A deterministic scene inside chunk (0,0), y in [0,16): a stone floor with a
// grass slab on top, a water pool, a glass block, and an air pocket. Enough to
// hit CubeBlockRenderer smooth+flat, FluidBlockRenderer, and face culling
// against both opaque and non-opaque neighbours.
void paintScene(Chunk& chunk) {
 const int stone = block::Block::STONE->id;
 const int grass = block::Block::GRASS_BLOCK->id;
 const int dirt = block::Block::DIRT->id;
 const int water = block::Block::WATER->id;
 const int glass = block::Block::GLASS->id;
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   for(int y = 0; y < 4; ++y) {
    chunk.blocks[static_cast<std::size_t>((x << 11) | (z << 7) | y)] = static_cast<std::uint8_t>(stone);
   }
   chunk.blocks[static_cast<std::size_t>((x << 11) | (z << 7) | 4)] = static_cast<std::uint8_t>(dirt);
   chunk.blocks[static_cast<std::size_t>((x << 11) | (z << 7) | 5)] = static_cast<std::uint8_t>(grass);
  }
 }
 // Water pool carved into the surface.
 for(int x = 2; x < 6; ++x) {
  for(int z = 2; z < 6; ++z) {
   chunk.blocks[static_cast<std::size_t>((x << 11) | (z << 7) | 5)] = static_cast<std::uint8_t>(water);
  }
 }
 chunk.blocks[static_cast<std::size_t>((7 << 11) | (7 << 7) | 9)] = static_cast<std::uint8_t>(water);
 // A glass block and a floating stone cube, both fully surrounded by air so
 // every face is visible and every AO corner is exercised.
 chunk.blocks[static_cast<std::size_t>((9 << 11) | (9 << 7) | 8)] = static_cast<std::uint8_t>(glass);
 chunk.blocks[static_cast<std::size_t>((12 << 11) | (4 << 7) | 9)] = static_cast<std::uint8_t>(stone);
 // Non-uniform light so a regression in light sampling or AO averaging shows
 // up as a hash change rather than being masked by a flat gradient.
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   for(int y = 0; y < 16; ++y) {
    chunk.skyLight.set(x, y, z, y >= 6 ? 15 : (y + x / 4) & 0xF);
    chunk.blockLight.set(x, y, z, (x * 3 + z * 5 + y) & 0xF);
   }
  }
 }
 chunk.populateHeightMapOnly();
}
std::array<float, 16> linearLuminance() {
 std::array<float, 16> table{};
 for(int level = 0; level < 16; ++level) {
  table[static_cast<std::size_t>(level)] = Dimension::luminanceForLightLevel(level);
 }
 return table;
}
struct MeshStats {
 std::size_t vertexCount = 0;
 std::uint64_t hash = 0;
 bool sawSkyLight = false;
 std::size_t fluidVertexCount = 0;
 std::size_t fluidColorCount = 0;
 std::size_t fluidSurfaceColorCount = 0;
 bool sawSampledFluidLight = false;
 std::size_t fluidTopQuadCount = 0;
 bool fluidTopUvCentered = true;
};
// FNV-1a over the raw vertex bytes. TessellatorVertex is a packed POD
// (static_assert'd at 28 bytes), so this covers position, UV, colour and
// normal exactly as they would be uploaded.
std::uint64_t hashMesh(const TessellatorMesh& mesh) {
 std::uint64_t hash = 1469598103934665603ULL;
 const auto* bytes = reinterpret_cast<const std::uint8_t*>(mesh.vertices.data());
 const std::size_t size = mesh.vertices.size() * sizeof(net::minecraft::client::render::TessellatorVertex);
 for(std::size_t i = 0; i < size; ++i) {
  hash ^= bytes[i];
  hash *= 1099511628211ULL;
 }
 return hash;
}
MeshStats meshLayer(int layer, bool ambientOcclusion, bool oldLighting = false) {
 auto chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 paintScene(*chunk);
 std::vector<RegionSnapshot::SourceChunk> sources{RegionSnapshot::SourceChunk{0, 0, chunk.get()}};
 RegionSnapshot snapshot(sources,
                         /*ambientDarkness=*/0,
                         linearLuminance(),
                         /*biomeSource=*/nullptr,
                         -1,
                         -1,
                         -1,
                         kSectionSize + 1,
                         kSectionSize,
                         kSectionSize + 1);
 client::option::RenderSettings opts{};
 opts.ambientOcclusionActive = ambientOcclusion;
 opts.ambientOcclusionStrength = ambientOcclusion ? 1.0f : 0.0f;
 opts.fancyLeaves = true;
 opts.fancyGrass = true;
 opts.renderWater = true;
 opts.fancyWater = true;
 opts.oldLighting = oldLighting;
 Tessellator tessellator;
 tessellator.setCaptureOnly(true);
 client::render::block::BlockRenderManager manager(tessellator, &snapshot, opts);
 bool began = false;
 for(int x = 0; x < kSectionSize; ++x) {
  for(int z = 0; z < kSectionSize; ++z) {
   for(int y = 0; y < kSectionSize; ++y) {
    const int blockId = snapshot.getBlockId(x, y, z);
    if(blockId <= 0) {
     continue;
    }
    block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
    if(block == nullptr || client::render::chunk::resolveTerrainMeshLayer(*block, blockId, {}) != layer) {
     continue;
    }
    if(!began) {
     began = true;
     tessellator.startQuads();
    }
    manager.render(*block, x, y, z);
   }
  }
 }
 MeshStats stats;
 if(began) {
  const TessellatorMesh mesh = tessellator.takeMesh();
  stats.vertexCount = mesh.vertices.size();
  stats.hash = hashMesh(mesh);
  std::unordered_set<std::uint32_t> fluidColors;
  std::unordered_set<std::uint32_t> fluidSurfaceColors;
  for(const auto& vertex : mesh.vertices) {
   if(vertex.entity[1] != 1) continue;
   ++stats.fluidVertexCount;
   fluidColors.insert(vertex.color);
   if(static_cast<std::int8_t>((vertex.normal >> 8) & 0xFF) >= 0) fluidSurfaceColors.insert(vertex.color);
   if(vertex.light != 0x00F000F0) stats.sawSampledFluidLight = true;
  }
  stats.fluidColorCount = fluidColors.size();
  stats.fluidSurfaceColorCount = fluidSurfaceColors.size();
  for(std::size_t i = 0; i + 3 < mesh.vertices.size(); i += 4) {
   const auto& first = mesh.vertices[i];
   if(first.entity[1] != 1 || static_cast<std::int8_t>((first.normal >> 8) & 0xFF) <= 0) continue;
   ++stats.fluidTopQuadCount;
   double midU = 0.0;
   double midV = 0.0;
   for(std::size_t j = 0; j < 4; ++j) {
    midU += mesh.vertices[i + j].u;
    midV += mesh.vertices[i + j].v;
   }
   const double tileU = std::fmod(midU * 64.0, 16.0);
   const double tileV = std::fmod(midV * 64.0, 16.0);
   stats.fluidTopUvCentered = stats.fluidTopUvCentered && std::abs(tileU - 8.0) < 0.01 &&
                              std::abs(tileV - 8.0) < 0.01;
  }
 }
 stats.sawSkyLight = snapshot.sawSkyLight();
 return stats;
}
} // namespace
TEST(ChunkMeshGolden, SolidLayerIsStableWithSmoothLighting) {
 const MeshStats stats = meshLayer(/*layer=*/0, /*ambientOcclusion=*/true);
 EXPECT_GT(stats.vertexCount, 0u);
 EXPECT_EQ(stats.vertexCount % 4, 0u) << "solid layer must be whole quads; the region buffer's "
                                         "index math and draw-range merging both assume it";
 EXPECT_TRUE(stats.sawSkyLight);
}
TEST(ChunkMeshGolden, SolidLayerIsStableWithFlatLighting) {
 const MeshStats stats = meshLayer(/*layer=*/0, /*ambientOcclusion=*/false);
 EXPECT_GT(stats.vertexCount, 0u);
 EXPECT_EQ(stats.vertexCount % 4, 0u);
}
// The whole point of W1: the AO branch must follow the snapshotted options, not
// a live global read off Minecraft::INSTANCE. Same geometry, different vertex
// colours, so vertex counts match while the hashes must differ.
TEST(ChunkMeshGolden, AmbientOcclusionFlagIsHonouredFromResolvedOptions) {
 const MeshStats smooth = meshLayer(/*layer=*/0, /*ambientOcclusion=*/true);
 const MeshStats flat = meshLayer(/*layer=*/0, /*ambientOcclusion=*/false);
 EXPECT_EQ(smooth.vertexCount, flat.vertexCount);
 EXPECT_NE(smooth.hash, flat.hash) << "smooth and flat lighting produced byte-identical meshes; the "
                                      "renderer is ignoring RenderSettings::ambientOcclusionActive";
}
TEST(ChunkMeshGolden, MeshingIsDeterministic) {
 const MeshStats first = meshLayer(/*layer=*/0, /*ambientOcclusion=*/true);
 const MeshStats second = meshLayer(/*layer=*/0, /*ambientOcclusion=*/true);
 EXPECT_EQ(first.vertexCount, second.vertexCount);
 EXPECT_EQ(first.hash, second.hash);
}
TEST(ChunkMeshGolden, TranslucentLayerEmitsWaterGeometry) {
 const MeshStats stats = meshLayer(/*layer=*/2, /*ambientOcclusion=*/true);
 EXPECT_GT(stats.vertexCount, 0u) << "the water pool should produce translucent-layer geometry";
 EXPECT_EQ(stats.vertexCount % 4, 0u);
 EXPECT_GT(stats.fluidVertexCount, 0u);
 EXPECT_TRUE(stats.sawSampledFluidLight);
 EXPECT_GE(stats.fluidColorCount, 1u);
 EXPECT_EQ(stats.fluidSurfaceColorCount, 1u);
 EXPECT_GT(stats.fluidTopQuadCount, 0u);
 EXPECT_TRUE(stats.fluidTopUvCentered);
}
TEST(ChunkMeshGolden, LegacyWaterLightingIsExplicitlyBakedOnlyWhenRequested) {
 const MeshStats modern = meshLayer(2, true, false);
 const MeshStats legacy = meshLayer(2, true, true);
 EXPECT_EQ(modern.fluidSurfaceColorCount, 1u);
 EXPECT_GT(legacy.fluidSurfaceColorCount, modern.fluidSurfaceColorCount);
}
TEST(ChunkMeshGolden, CutoutLayerEmitsNonOpaqueGeometry) {
 const MeshStats stats = meshLayer(/*layer=*/1, /*ambientOcclusion=*/true);
 EXPECT_GT(stats.vertexCount, 0u);
 EXPECT_EQ(stats.vertexCount % 4, 0u);
}
TEST(TessellatorVertexAbi, EncodesOnlyFluidGeometryAsFluid) {
 const auto encode = [](bool fluid) {
  Tessellator tessellator;
  tessellator.setCaptureOnly(true);
  tessellator.startQuads();
  tessellator.blockData(0.0, 0.0, 0.0, 0, 15, 15, -1, fluid, 0);
  tessellator.vertex(0.0, 0.0, 0.0);
  tessellator.vertex(1.0, 0.0, 0.0);
  tessellator.vertex(1.0, 1.0, 0.0);
  tessellator.vertex(0.0, 1.0, 0.0);
  return tessellator.takeMesh();
 };
 const TessellatorMesh solid = encode(false);
 const TessellatorMesh fluid = encode(true);
 ASSERT_EQ(solid.vertices.size(), 4u);
 ASSERT_EQ(fluid.vertices.size(), 4u);
 for(const auto& vertex : solid.vertices) EXPECT_EQ(vertex.entity[1], -1);
 for(const auto& vertex : fluid.vertices) EXPECT_EQ(vertex.entity[1], 1);
 EXPECT_EQ(static_cast<std::int8_t>(solid.vertices[0].midBlock & 0xFF), 32);
 EXPECT_EQ(static_cast<std::int8_t>((solid.vertices[0].midBlock >> 8) & 0xFF), 32);
 EXPECT_EQ(static_cast<std::int8_t>((solid.vertices[0].midBlock >> 16) & 0xFF), 32);
 EXPECT_EQ(static_cast<std::int8_t>(solid.vertices[1].midBlock & 0xFF), -32);
}
// RegionSnapshot deliberately reports out-of-range cells as fully sky-lit air
// rather than dark, so neighbour-light sampling does not bake a dark fringe at
// section borders. The flat-shell rewrite must preserve this exactly.
TEST(ChunkMeshGolden, OutOfRangeReadsAreAirAndSkyLit) {
 auto chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 paintScene(*chunk);
 std::vector<RegionSnapshot::SourceChunk> sources{RegionSnapshot::SourceChunk{0, 0, chunk.get()}};
 RegionSnapshot snapshot(
     sources, /*ambientDarkness=*/0, linearLuminance(), nullptr, -1, -1, -1, kSectionSize + 1, kSectionSize,
     kSectionSize + 1);
 // Far outside the captured columns.
 EXPECT_EQ(snapshot.getBlockId(500, 8, 500), 0);
 EXPECT_EQ(snapshot.getRawBrightness(500, 8, 500, true), 15);
 // Below and above the world.
 EXPECT_EQ(snapshot.getBlockId(0, -1, 0), 0);
 EXPECT_EQ(snapshot.getRawBrightness(0, Chunk::height + 4, 0, true), 15);
}
// BLOCK light has no such optimistic default — an absent column reads 0, which
// is vanilla's EnumSkyBlock.Block.defaultLightValue. That is correct parity, and
// it is exactly why a section meshed while a neighbour column was missing bakes
// a dark fringe that only a re-mesh can remove.
//
// The columns a mesh job depends on are the full 3x3, DIAGONALS INCLUDED:
// ChunkBuilder captures `owner.x - 1 .. owner.x + kSectionBlocks + 1`, and
// averageCornerLight samples the diagonal cell of every corner, so a block in a
// section's corner reads the diagonal chunk. ChunkSectionSystem::drainBorderRefresh
// must therefore invalidate all eight surrounding columns when a chunk loads;
// refreshing only the four orthogonal ones leaves the corners dark forever,
// because nothing else ever invalidates them.
TEST(ChunkMeshGolden, CornerBlockLightComesFromTheDiagonalColumn) {
 auto centre = std::make_unique<Chunk>(nullptr, 0, 0);
 paintScene(*centre);
 auto diagonal = std::make_unique<Chunk>(nullptr, -1, -1);
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   for(int y = 0; y < 16; ++y) {
    diagonal->blockLight.set(x, y, z, 15);
   }
  }
 }
 diagonal->populateHeightMapOnly();
 const auto cornerBlockLight = [](std::span<const RegionSnapshot::SourceChunk> sources) {
  RegionSnapshot snapshot(sources, /*ambientDarkness=*/0, linearLuminance(), nullptr, -1, -1, -1,
                          kSectionSize + 1, kSectionSize, kSectionSize + 1);
  // The diagonal probe averageCornerLight reads for the corner block (0, y, 0).
  return snapshot.getBlockLight(-1, 4, -1);
 };
 const RegionSnapshot::SourceChunk centreOnly[] = {{0, 0, centre.get()}};
 const RegionSnapshot::SourceChunk withDiagonal[] = {{0, 0, centre.get()}, {-1, -1, diagonal.get()}};
 EXPECT_EQ(cornerBlockLight(centreOnly), 0)
     << "an absent column must read as unlit, matching vanilla's block-light default";
 EXPECT_EQ(cornerBlockLight(withDiagonal), 15)
     << "the diagonal column feeds this section's corner light, so it is a real mesh dependency";
}
} // namespace net::minecraft::test
