// separateAo=true (Iris shaders.properties) moves the ambient-occlusion
// coefficient out of the vertex colour's RGB and into its ALPHA channel — see
// the Iris mixin doc comment on MixinBufferBuilder_SeparateAo: "directional
// shading and ambient occlusion lighting coefficients are pre-multiplied into
// the vertex color RGB... Since the alpha field of the vertex color is unused
// for blocks, it is possible to use the alpha field to store the directional
// shading / ambient occlusion coefficient for each vertex."
//
// Two things that must hold, and each has been broken here before:
//
//  1. Alpha must actually vary with geometry. It was hardcoded to 1.0, so every
//     corner reported zero occlusion and a pack shading from it got one flat
//     value pack-wide.
//
//  2. Alpha must be GEOMETRIC — a function of which neighbours are opaque, and
//     of nothing else. A later fix derived it from a corner-vs-flat LIGHT ratio
//     instead, which makes AO a function of the time of day and of every torch
//     placement. Packs feed alpha into much more than a brightness multiply
//     (rethinking-voxels: `vanillaAO = glColor.a` gating all diffuse through
//     pow2, `centerFactor = max(glColor.a, lightmapYM)` steering the shadow
//     sample position, and `dFdx(glColor.a)` as an AO gradient), so an alpha
//     that moves with the light reads on screen as lighting that shifts, seams
//     and flickers every time the light engine republishes a region.
//
// The lightmap is NOT part of the separateAo split: in Java the per-corner
// light average and the per-corner AO average are two independent values that
// are both present on every vertex either way.
//
// This drives BlockRenderManager end-to-end against a real chunk (same pattern
// as chunk_mesh_golden_test.cpp) rather than unit-testing the internal helper
// directly, since assignAoCorners is file-local.
#include <gtest/gtest.h>
#include <algorithm>
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
using client::render::chunk::RegionSnapshot;
constexpr int kSectionSize = 16;
// A stone floor with one fully-exposed floating stone cube (every face visible,
// every AO corner open) and a solid concave corner formed by an L-shaped stack.
// The geometry is fixed; only the light field varies between scenes, which is
// what lets the light-independence test attribute any alpha difference to the
// light alone.
void paintGeometry(Chunk& chunk) {
 const int stone = block::Block::STONE->id;
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   chunk.blocks[static_cast<std::size_t>((x << 11) | (z << 7) | 0)] = static_cast<std::uint8_t>(stone);
  }
 }
 // Floating cube: fully surrounded by air, so every corner of every face is
 // open (no occluding neighbour) — every corner should report LOW occlusion.
 chunk.blocks[static_cast<std::size_t>((8 << 11) | (8 << 7) | 6)] = static_cast<std::uint8_t>(stone);
 // A concave inner corner: an L-shaped stack enclosing one corner of the step
 // between two solid neighbours — the shape corner AO exists to darken.
 chunk.blocks[static_cast<std::size_t>((3 << 11) | (3 << 7) | 1)] = static_cast<std::uint8_t>(stone);
 chunk.blocks[static_cast<std::size_t>((4 << 11) | (3 << 7) | 1)] = static_cast<std::uint8_t>(stone);
 chunk.blocks[static_cast<std::size_t>((3 << 11) | (4 << 7) | 1)] = static_cast<std::uint8_t>(stone);
 chunk.blocks[static_cast<std::size_t>((3 << 11) | (3 << 7) | 2)] = static_cast<std::uint8_t>(stone);
}
// Daylight outdoors: sky light everywhere above the floor, no block light.
void paintDaylight(Chunk& chunk) {
 paintGeometry(chunk);
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   for(int y = 0; y < 16; ++y) {
    chunk.skyLight.set(x, y, z, y >= 6 ? 15 : (y + x / 4) & 0xF);
    chunk.blockLight.set(x, y, z, 0);
   }
  }
 }
 chunk.populateHeightMapOnly();
}
// The same geometry at night, indoors, lit only by torches — and with frequent
// literal zeros next to well-lit neighbours, imitating an AO corner probe
// landing inside solid rock. Under the light-ratio AO this produced wildly
// different (and near-zero) alphas from the identical geometry above.
void paintTorchlit(Chunk& chunk) {
 paintGeometry(chunk);
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   for(int y = 0; y < 16; ++y) {
    chunk.skyLight.set(x, y, z, 0); // fully indoors: no sky access anywhere.
    chunk.blockLight.set(x, y, z, ((x * 7 + y * 13 + z * 19) % 4 == 0) ? 0 : 12);
   }
  }
 }
 chunk.populateHeightMapOnly();
}
std::array<float, 16> linearLuminance() {
 std::array<float, 16> table{};
 for(int level = 0; level < 16; ++level)
  table[static_cast<std::size_t>(level)] = Dimension::luminanceForLightLevel(level);
 return table;
}
struct MeshedScene {
 std::unique_ptr<Chunk> chunk;
 std::unique_ptr<RegionSnapshot> snapshot;
 TessellatorMesh mesh;
};
MeshedScene meshScene(bool separateAo,
                      float aoStrength = 1.0f,
                      void (*paint)(Chunk&) = paintDaylight,
                      bool oldLighting = false) {
 MeshedScene scene;
 scene.chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 paint(*scene.chunk);
 std::vector<RegionSnapshot::SourceChunk> sources{RegionSnapshot::SourceChunk{0, 0, scene.chunk.get()}};
 scene.snapshot = std::make_unique<RegionSnapshot>(sources, /*ambientDarkness=*/0, linearLuminance(),
                                                   /*biomeSource=*/nullptr, -1, -1, -1, kSectionSize + 1,
                                                   kSectionSize, kSectionSize + 1);
 client::option::RenderSettings opts{};
 opts.ambientOcclusionActive = true;
 opts.ambientOcclusionStrength = aoStrength;
 opts.separateAo = separateAo;
 opts.oldLighting = oldLighting;
 Tessellator tessellator;
 tessellator.setCaptureOnly(true);
 client::render::block::BlockRenderManager manager(tessellator, scene.snapshot.get(), opts);
 tessellator.startQuads();
 for(int x = 0; x < kSectionSize; ++x) {
  for(int z = 0; z < kSectionSize; ++z) {
   for(int y = 0; y < kSectionSize; ++y) {
    const int blockId = scene.snapshot->getBlockId(x, y, z);
    if(blockId <= 0) continue;
    block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
    if(block == nullptr) continue;
    manager.render(*block, x, y, z);
   }
  }
 }
 scene.mesh = tessellator.takeMesh();
 return scene;
}
float alphaOf(const client::render::TessellatorVertex& v) {
 return static_cast<float>((v.color >> 24) & 0xFF) / 255.0f;
}
} // namespace
// With separateAo=true, alpha must NOT be a single constant across the whole
// mesh: it was hardcoded to 1.0 regardless of geometry.
TEST(SeparateAoAlpha, AlphaVariesAcrossCornersWhenSeparateAoEnabled) {
 const MeshedScene scene = meshScene(/*separateAo=*/true);
 ASSERT_FALSE(scene.mesh.vertices.empty());
 float minAlpha = 1.0f;
 float maxAlpha = 0.0f;
 for(const auto& v : scene.mesh.vertices) {
  minAlpha = std::min(minAlpha, alphaOf(v));
  maxAlpha = std::max(maxAlpha, alphaOf(v));
 }
 EXPECT_LT(minAlpha, maxAlpha - 0.01f)
     << "alpha is constant across the mesh; separateAo is not carrying per-corner occlusion";
}
// Regression guard for the OTHER mode: separateAo=false must keep alpha at 1.0.
TEST(SeparateAoAlpha, AlphaStaysConstantWhenSeparateAoDisabled) {
 const MeshedScene scene = meshScene(/*separateAo=*/false);
 ASSERT_FALSE(scene.mesh.vertices.empty());
 for(const auto& v : scene.mesh.vertices) {
  EXPECT_FLOAT_EQ(alphaOf(v), 1.0f);
 }
}
// THE regression this file exists for. Identical geometry, two completely
// different light fields (bright daylight vs. a dark torch-lit room with
// zero-light probes): every vertex must report the SAME occlusion, because AO
// is geometry. When alpha was a corner-vs-flat light ratio this failed hard,
// and on screen it was lighting that shifted and flickered as the light engine
// republished regions — plus a hard seam wherever the ratio's `skyLight > 0`
// branch flipped to the block-light branch mid-surface.
TEST(SeparateAoAlpha, AlphaIsGeometryOnlyAndDoesNotMoveWithTheLight) {
 const MeshedScene daylight = meshScene(/*separateAo=*/true, /*aoStrength=*/1.0f, paintDaylight);
 const MeshedScene torchlit = meshScene(/*separateAo=*/true, /*aoStrength=*/1.0f, paintTorchlit);
 ASSERT_FALSE(daylight.mesh.vertices.empty());
 ASSERT_EQ(daylight.mesh.vertices.size(), torchlit.mesh.vertices.size())
     << "the two scenes must differ only in light, so they must mesh to the same vertex list";
 bool lightActuallyDiffered = false;
 for(std::size_t i = 0; i < daylight.mesh.vertices.size(); ++i) {
  const auto& day = daylight.mesh.vertices[i];
  const auto& night = torchlit.mesh.vertices[i];
  if(day.light != night.light) lightActuallyDiffered = true;
  EXPECT_NEAR(alphaOf(day), alphaOf(night), 1.0f / 255.0f)
      << "vertex " << i << " changed its AO because the LIGHT changed; AO must be geometric";
 }
 EXPECT_TRUE(lightActuallyDiffered)
     << "the two scenes ended up with identical lightmaps, so this test proved nothing";
}
// The comparative claim behind the fix: a corner enclosed by solid neighbours
// (the concave L-corner) must be measurably more occluded than a corner on a
// fully exposed, floating block. This is the 'closed inner corner' path
// documented on averageCornerLight (the diagonal sample is dropped when both
// edges are opaque).
TEST(SeparateAoAlpha, EnclosedCornerIsMoreOccludedThanAnExposedCorner) {
 const MeshedScene scene = meshScene(/*separateAo=*/true);
 float minAlphaNearConcaveCorner = 1.0f;
 float minAlphaOnFloatingCube = 1.0f;
 bool sawConcave = false;
 bool sawFloating = false;
 for(const auto& v : scene.mesh.vertices) {
  // The concave step occupies x in [3,5), z in [3,5), y in [1,3).
  if(v.x >= 2.99f && v.x <= 5.01f && v.z >= 2.99f && v.z <= 5.01f && v.y >= 0.99f && v.y <= 3.01f) {
   sawConcave = true;
   minAlphaNearConcaveCorner = std::min(minAlphaNearConcaveCorner, alphaOf(v));
  }
  // The floating cube occupies x in [8,9), z in [8,9), y in [6,7).
  if(v.x >= 7.99f && v.x <= 9.01f && v.z >= 7.99f && v.z <= 9.01f && v.y >= 5.99f && v.y <= 7.01f) {
   sawFloating = true;
   minAlphaOnFloatingCube = std::min(minAlphaOnFloatingCube, alphaOf(v));
  }
 }
 ASSERT_TRUE(sawConcave) << "test scene geometry assumption is wrong; fix the coordinates above";
 ASSERT_TRUE(sawFloating) << "test scene geometry assumption is wrong; fix the coordinates above";
 EXPECT_LT(minAlphaNearConcaveCorner, minAlphaOnFloatingCube)
     << "an enclosed corner should be more occluded (lower alpha) than an exposed one";
}
// With the AO strength slider at 0, separateAo must also report "no occlusion":
// the slider is an engine-wide setting, not a shaderpack-only one.
TEST(SeparateAoAlpha, ZeroAoStrengthMeansNoOcclusionEvenWhenSeparate) {
 const MeshedScene scene = meshScene(/*separateAo=*/true, /*aoStrength=*/0.0f);
 ASSERT_FALSE(scene.mesh.vertices.empty());
 for(const auto& v : scene.mesh.vertices) {
  EXPECT_NEAR(alphaOf(v), 1.0f, 1.0f / 255.0f);
 }
}
// The exact Java statement this mode implements — Iris'
// XHFPTerrainVertex.write:
//
//   MemoryAccess.setInt(ptr + 8L, WorldRenderingSettings.INSTANCE.shouldUseSeparateAo()
//       ? ColorABGR.withAlpha(vertex.color, vertex.ao)
//       : ColorARGB.mulRGB(vertex.color, vertex.ao));
//
TEST(SeparateAoAlpha, CombinedAoPremultipliesRgb) {
 const MeshedScene separate = meshScene(/*separateAo=*/true);
 const MeshedScene combined = meshScene(/*separateAo=*/false);
 ASSERT_FALSE(separate.mesh.vertices.empty());
 ASSERT_EQ(separate.mesh.vertices.size(), combined.mesh.vertices.size());
 bool sawOccludedRgb = false;
 for(std::size_t i = 0; i < separate.mesh.vertices.size(); ++i) {
  const std::uint32_t separated = separate.mesh.vertices[i].color;
  const std::uint32_t merged = combined.mesh.vertices[i].color;
  for(const int shift : {0, 8, 16}) {
   const std::uint32_t separateChannel = (separated >> shift) & 0xFFu;
   const std::uint32_t combinedChannel = (merged >> shift) & 0xFFu;
   EXPECT_LE(combinedChannel, separateChannel) << "vertex " << i << ", channel " << shift;
   sawOccludedRgb = sawOccludedRgb || combinedChannel < separateChannel;
  }
  EXPECT_EQ((merged >> 24) & 0xFFu, 0xFFu) << "vertex " << i;
 }
 EXPECT_TRUE(sawOccludedRgb);
}

TEST(SeparateAoAlpha, OldLightingUsesTheSameIrisColorSplit) {
 const MeshedScene separate = meshScene(true, 1.0f, paintDaylight, true);
 const MeshedScene combined = meshScene(false, 1.0f, paintDaylight, true);
 ASSERT_FALSE(separate.mesh.vertices.empty());
 ASSERT_EQ(separate.mesh.vertices.size(), combined.mesh.vertices.size());
 bool sawDirectionalShade = false;
 for(std::size_t i = 0; i < separate.mesh.vertices.size(); ++i) {
  const std::uint32_t separated = separate.mesh.vertices[i].color;
  const std::uint32_t merged = combined.mesh.vertices[i].color;
  const std::uint32_t coefficient = (separated >> 24) & 0xFFu;
  sawDirectionalShade = sawDirectionalShade || coefficient < 0xFFu;
  for(const int shift : {0, 8, 16}) {
   const std::uint32_t tint = (separated >> shift) & 0xFFu;
   const std::uint32_t expected = (tint * coefficient + 127u) / 255u;
   const std::uint32_t actual = (merged >> shift) & 0xFFu;
   EXPECT_NEAR(static_cast<float>(actual), static_cast<float>(expected), 1.0f) << "vertex " << i;
  }
 }
 EXPECT_TRUE(sawDirectionalShade);
}
// separateAo splits the vertex COLOUR, not the light. The lightmap must be the
// same vanilla corner-averaged light in both modes — flattening it to the
// face-adjacent light (as the light-ratio version did, to avoid "double
// darkening") throws smooth lighting away entirely, which is what turned
// block-to-block light steps into hard visible seams.
TEST(SeparateAoAlpha, LightmapIsUnaffectedBySeparateAo) {
 const MeshedScene separate = meshScene(/*separateAo=*/true);
 const MeshedScene combined = meshScene(/*separateAo=*/false);
 ASSERT_FALSE(separate.mesh.vertices.empty());
 ASSERT_EQ(separate.mesh.vertices.size(), combined.mesh.vertices.size());
 for(std::size_t i = 0; i < separate.mesh.vertices.size(); ++i) {
  EXPECT_EQ(separate.mesh.vertices[i].light, combined.mesh.vertices[i].light)
      << "vertex " << i << ": separateAo changed the lightmap; it must only move AO into alpha";
 }
}
// Smooth lighting must survive: a face whose corners see different light must
// carry different per-corner lightmap values. A flat per-face lightmap is what
// makes lit terrain look like it is made of hard-edged tiles.
TEST(SeparateAoAlpha, LightmapKeepsPerCornerVariationUnderSeparateAo) {
 const MeshedScene scene = meshScene(/*separateAo=*/true);
 bool sawVariationWithinAQuad = false;
 for(std::size_t i = 0; i + 3 < scene.mesh.vertices.size(); i += 4) {
  const std::int32_t first = scene.mesh.vertices[i].light;
  for(std::size_t c = 1; c < 4; ++c) {
   if(scene.mesh.vertices[i + c].light != first) sawVariationWithinAQuad = true;
  }
 }
 EXPECT_TRUE(sawVariationWithinAQuad)
     << "every quad has one uniform lightmap value; smooth lighting was flattened away";
}
// Vanilla's AO coefficient is bounded (b1.7.3 Block.getAmbientOcclusionLightValue
// returns 0.2 for an opaque cube and 1.0 otherwise, averaged over four samples),
// so alpha can never be a literal 0. Packs multiply it straight into final
// diffuse — RenderPearl runs it through smoothstep(0.05, 0.8, x) * x — and a
// literal 0 there renders whole faces solid black.
TEST(SeparateAoAlpha, AlphaStaysWithinVanillasBoundedOcclusionRange) {
 for(void (*paint)(Chunk&) : {paintDaylight, paintTorchlit}) {
  const MeshedScene scene = meshScene(/*separateAo=*/true, /*aoStrength=*/1.0f, paint);
  ASSERT_FALSE(scene.mesh.vertices.empty());
  for(const auto& v : scene.mesh.vertices) {
   EXPECT_GE(alphaOf(v), 0.2f - (1.0f / 255.0f)) << "alpha fell below vanilla's 0.2 occlusion floor";
   EXPECT_LE(alphaOf(v), 1.0f);
  }
 }
}
} // namespace net::minecraft::test
