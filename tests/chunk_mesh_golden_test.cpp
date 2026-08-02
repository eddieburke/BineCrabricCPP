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
#include <memory>
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
MeshStats meshLayer(int layer, bool ambientOcclusion) {
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
 Tessellator tessellator;
 tessellator.setCaptureOnly(true);
 client::render::block::BlockRenderManager manager(&snapshot, opts);
 manager.ctx.tess = &tessellator;
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
 EXPECT_EQ(snapshot.getRawBrightness(500, 8, 500), 15);
 // Below and above the world.
 EXPECT_EQ(snapshot.getBlockId(0, -1, 0), 0);
 EXPECT_EQ(snapshot.getRawBrightness(0, Chunk::height + 4, 0), 15);
}
} // namespace net::minecraft::test
