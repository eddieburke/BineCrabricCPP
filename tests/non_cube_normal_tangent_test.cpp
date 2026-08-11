// Normals and tangents for the block renderers that DON'T call normal().
//
// Only BlockFaceRenderer, CrossBlockRenderer and FluidBlockRenderer emit through
// emitBlockVertex. Crops, torches, ladders, rails, redstone dust, repeaters,
// levers, fire, beds and pistons call Tessellator::vertex directly, so their
// vertices carry whatever the tessellator last had — which, inside a chunk mesh
// that already emitted cube faces, is the last cube face's normal.
//
// Java has no notion of a producer-supplied terrain normal surviving: while the
// level is rendering it recomputes the face normal for every quad
// (MixinBufferBuilder: `recalculateNormal = ImmediateState.isRenderingLevel`,
// then `NormalHelper.computeFaceNormal(normal, polygon)` written over the
// vertex normal). These tests drive the real BlockRenderManager over a real
// crop block to check that, and pin the tangent maths against a direct
// transcription of NormalHelper.computeTangent.
#include <gtest/gtest.h>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
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
struct Vec3 {
 float x = 0.0f;
 float y = 0.0f;
 float z = 0.0f;
};
Vec3 normalOf(const TessellatorVertex& v) {
 return {static_cast<float>(static_cast<std::int8_t>(v.normal & 0xFF)) / 127.0f,
         static_cast<float>(static_cast<std::int8_t>((v.normal >> 8) & 0xFF)) / 127.0f,
         static_cast<float>(static_cast<std::int8_t>((v.normal >> 16) & 0xFF)) / 127.0f};
}
std::array<float, 16> linearLuminance() {
 std::array<float, 16> table{};
 for(int level = 0; level < 16; ++level)
  table[static_cast<std::size_t>(level)] = Dimension::luminanceForLightLevel(level);
 return table;
}
// A stone floor with dirt under a wheat crop, plus surrounding stone cubes so
// the mesh emits CUBE faces (which call normal()) BEFORE the crop in the same
// tessellator part — the exact ordering that defeats any "did anyone set a
// normal" flag.
void paintCropField(Chunk& chunk) {
 const int stone = block::Block::STONE->id;
 const int dirt = block::Block::DIRT->id;
 const int wheat = block::Block::WHEAT->id;
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   chunk.blocks[static_cast<std::size_t>((x << 11) | (z << 7) | 0)] = static_cast<std::uint8_t>(stone);
   chunk.blocks[static_cast<std::size_t>((x << 11) | (z << 7) | 1)] = static_cast<std::uint8_t>(dirt);
  }
 }
 // Crops sit at y = 2, above the dirt.
 for(int x = 4; x < 8; ++x) {
  for(int z = 4; z < 8; ++z) {
   chunk.blocks[static_cast<std::size_t>((x << 11) | (z << 7) | 2)] = static_cast<std::uint8_t>(wheat);
  }
 }
 for(int x = 0; x < 16; ++x) {
  for(int z = 0; z < 16; ++z) {
   for(int y = 0; y < 16; ++y) {
    chunk.skyLight.set(x, y, z, 15);
    chunk.blockLight.set(x, y, z, 0);
   }
  }
 }
 chunk.populateHeightMapOnly();
}
struct Scene {
 std::unique_ptr<Chunk> chunk;
 std::unique_ptr<RegionSnapshot> snapshot;
 TessellatorMesh mesh;
};
Scene meshEverything() {
 Scene scene;
 scene.chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 paintCropField(*scene.chunk);
 std::vector<RegionSnapshot::SourceChunk> sources{RegionSnapshot::SourceChunk{0, 0, scene.chunk.get()}};
 scene.snapshot = std::make_unique<RegionSnapshot>(sources, /*ambientDarkness=*/0, linearLuminance(),
                                                   /*biomeSource=*/nullptr, -1, -1, -1, kSectionSize + 1,
                                                   kSectionSize, kSectionSize + 1);
 client::option::RenderSettings opts{};
 opts.ambientOcclusionActive = true;
 opts.ambientOcclusionStrength = 1.0f;
 Tessellator tessellator;
 tessellator.setCaptureOnly(true);
 client::render::block::BlockRenderManager manager(tessellator, scene.snapshot.get(), opts);
 tessellator.startQuads();
 // Cubes first, crops second — the real mesher walks x/z/y and hits the floor
 // before the crops above it, but ordering it explicitly makes the dependency
 // the test is about impossible to lose to a loop-order change.
 for(int pass = 0; pass < 2; ++pass) {
  for(int x = 0; x < kSectionSize; ++x) {
   for(int z = 0; z < kSectionSize; ++z) {
    for(int y = 0; y < kSectionSize; ++y) {
     const int blockId = scene.snapshot->getBlockId(x, y, z);
     if(blockId <= 0) continue;
     block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
     if(block == nullptr) continue;
     const bool isCrop = blockId == block::Block::WHEAT->id;
     if((pass == 0) == isCrop) continue;
     manager.render(*block, x, y, z);
    }
   }
  }
 }
 scene.mesh = tessellator.takeMesh();
 return scene;
}
// Crop geometry is the only thing in this scene on non-integer coordinates:
// CropBlockRenderer draws its planes at `y - 0.0625` and spans one block, so
// every crop vertex sits at y = 1.9375 or 2.9375, while every cube vertex is on
// a whole number.
//
// A y-range filter does NOT work here and the first version of this test was
// wrong for exactly that reason: the dirt block's TOP face is at y = 2.0, right
// in the middle of the crops, and it correctly reports (0, 1, 0). Selecting
// "everything above the dirt" swept those cube faces in and read their honest
// up-normal as a crop that had inherited one.
bool isCropVertex(const TessellatorVertex& v) {
 return std::fabs(v.y - std::round(v.y)) > 0.01f;
}
} // namespace
// THE bug. A crop's quads are vertical, so no vertex of one may report a
// straight-up normal — neither the (0,1,0) prototype nor the +Y of the dirt
// block's top face that was emitted into the same part just before it.
TEST(NonCubeNormals, CropQuadsAreVerticalNotTheInheritedCubeNormal) {
 const Scene scene = meshEverything();
 ASSERT_FALSE(scene.mesh.vertices.empty());
 std::size_t cropVertices = 0;
 for(const auto& v : scene.mesh.vertices) {
  if(!isCropVertex(v)) continue;
  ++cropVertices;
  const Vec3 n = normalOf(v);
  EXPECT_LT(std::fabs(n.y), 0.1f)
      << "a crop quad reported a vertical normal (" << n.x << ", " << n.y << ", " << n.z
      << "); it either kept the prototype or inherited the preceding cube face";
  EXPECT_NEAR(std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z), 1.0f, 2.0f / 127.0f) << "not unit length";
 }
 EXPECT_GT(cropVertices, 0U) << "no crop geometry was emitted; the scene assumption is wrong";
}
// CropBlockRenderer emits each plane twice with reversed winding so it is
// visible from both sides. Each of those must face its own way, which only
// works if the normal comes from the winding.
TEST(NonCubeNormals, CropFrontAndBackQuadsFaceOppositeWays) {
 const Scene scene = meshEverything();
 bool sawPositiveX = false, sawNegativeX = false, sawPositiveZ = false, sawNegativeZ = false;
 for(const auto& v : scene.mesh.vertices) {
  if(!isCropVertex(v)) continue;
  const Vec3 n = normalOf(v);
  if(n.x > 0.9f) sawPositiveX = true;
  if(n.x < -0.9f) sawNegativeX = true;
  if(n.z > 0.9f) sawPositiveZ = true;
  if(n.z < -0.9f) sawNegativeZ = true;
 }
 EXPECT_TRUE(sawPositiveX && sawNegativeX) << "the X-facing crop planes do not come in opposing pairs";
 EXPECT_TRUE(sawPositiveZ && sawNegativeZ) << "the Z-facing crop planes do not come in opposing pairs";
}
// Cube faces still come out axis-aligned: the recomputation must agree with the
// hand-written normals it now overwrites.
TEST(NonCubeNormals, CubeFacesInTheSameMeshRemainAxisAligned) {
 const Scene scene = meshEverything();
 std::size_t checked = 0;
 for(const auto& v : scene.mesh.vertices) {
  if(isCropVertex(v)) continue;
  const Vec3 n = normalOf(v);
  const float axisSum = std::fabs(n.x) + std::fabs(n.y) + std::fabs(n.z);
  EXPECT_NEAR(axisSum, 1.0f, 2.0f / 127.0f) << "cube face normal is not axis aligned";
  ++checked;
 }
 EXPECT_GT(checked, 0U);
}
// ---- fillMidTexAndTangent against NormalHelper ---------------------------
namespace {
// Direct transcription of Java NormalHelper.computeTangent's handedness, using
// the first three corners of the quad the way MixinBufferBuilder does.
float javaTangentW(const TessellatorVertex& v0,
                   const TessellatorVertex& v1,
                   const TessellatorVertex& v2,
                   const Vec3& normal) {
 const float edge1[3] = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
 const float edge2[3] = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
 const float deltaU1 = v1.u - v0.u, deltaV1 = v1.v - v0.v;
 const float deltaU2 = v2.u - v0.u, deltaV2 = v2.v - v0.v;
 const float fdenom = deltaU1 * deltaV2 - deltaU2 * deltaV1;
 const float f = fdenom == 0.0f ? 1.0f : 1.0f / fdenom;
 float tangent[3] = {f * (deltaV2 * edge1[0] - deltaV1 * edge2[0]),
                     f * (deltaV2 * edge1[1] - deltaV1 * edge2[1]),
                     f * (deltaV2 * edge1[2] - deltaV1 * edge2[2])};
 const float tlen = std::sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]);
 for(float& t : tangent) t /= tlen;
 float bitangent[3] = {f * (-deltaU2 * edge1[0] + deltaU1 * edge2[0]),
                       f * (-deltaU2 * edge1[1] + deltaU1 * edge2[1]),
                       f * (-deltaU2 * edge1[2] + deltaU1 * edge2[2])};
 const float blen =
     std::sqrt(bitangent[0] * bitangent[0] + bitangent[1] * bitangent[1] + bitangent[2] * bitangent[2]);
 for(float& b : bitangent) b /= blen;
 // predicted bitangent = tangent x normal
 const float pb[3] = {tangent[1] * normal.z - tangent[2] * normal.y,
                      tangent[2] * normal.x - tangent[0] * normal.z,
                      tangent[0] * normal.y - tangent[1] * normal.x};
 const float dot = bitangent[0] * pb[0] + bitangent[1] * pb[1] + bitangent[2] * pb[2];
 return dot < 0.0f ? -1.0f : 1.0f;
}
} // namespace
// The handedness sign. This was computed from `normal x tangent` — the opposite
// vector to Java's `tangent x normal` — so every w came out inverted, mirroring
// the bitangent of every TBN basis a pack builds from it.
TEST(QuadTangent, HandednessMatchesNormalHelper) {
 const Scene scene = meshEverything();
 ASSERT_GE(scene.mesh.vertices.size(), 4U);
 ASSERT_EQ(scene.mesh.vertices.size() % 4U, 0U) << "capture-only meshes are quads";
 std::size_t checked = 0;
 for(std::size_t q = 0; q + 3 < scene.mesh.vertices.size(); q += 4) {
  const TessellatorVertex& v0 = scene.mesh.vertices[q];
  const Vec3 n = normalOf(v0);
  if(std::fabs(n.x) + std::fabs(n.y) + std::fabs(n.z) < 0.5f) continue;
  const float expected = javaTangentW(v0, scene.mesh.vertices[q + 1], scene.mesh.vertices[q + 2], n);
  const float actual = static_cast<float>(v0.tangent[3]) / 32767.0f;
  if(!std::isfinite(expected)) continue; // degenerate UVs; Java produces NaN there, the engine clamps
  EXPECT_NEAR(actual, expected, 1.0e-3f) << "quad " << q / 4 << ": handedness sign disagrees with Java";
  ++checked;
 }
 EXPECT_GT(checked, 0U);
}
// Every vertex of a quad shares one tangent, and it is a unit vector — the
// basis is per-face, not per-vertex.
TEST(QuadTangent, AllFourVerticesShareOneUnitTangent) {
 const Scene scene = meshEverything();
 for(std::size_t q = 0; q + 3 < scene.mesh.vertices.size(); q += 4) {
  for(std::size_t c = 1; c < 4; ++c) {
   for(int axis = 0; axis < 4; ++axis) {
    EXPECT_EQ(scene.mesh.vertices[q + c].tangent[axis], scene.mesh.vertices[q].tangent[axis])
        << "quad " << q / 4 << " axis " << axis;
   }
  }
  const float tx = static_cast<float>(scene.mesh.vertices[q].tangent[0]) / 32767.0f;
  const float ty = static_cast<float>(scene.mesh.vertices[q].tangent[1]) / 32767.0f;
  const float tz = static_cast<float>(scene.mesh.vertices[q].tangent[2]) / 32767.0f;
  EXPECT_NEAR(std::sqrt(tx * tx + ty * ty + tz * tz), 1.0f, 1.0e-3f) << "quad " << q / 4;
 }
}
// The handedness must be exactly +1 or -1: packs multiply the bitangent by it,
// so any other magnitude scales the basis.
TEST(QuadTangent, HandednessIsExactlyPlusOrMinusOne) {
 const Scene scene = meshEverything();
 for(const auto& v : scene.mesh.vertices) {
  EXPECT_EQ(std::abs(static_cast<int>(v.tangent[3])), 32767) << "handedness is not a unit sign";
 }
}
// midU/midV are the average of the polygon's own UVs, written to every vertex —
// Java accumulates `midU += polygon.u(vertex)` over the quad and divides by the
// vertex count.
TEST(QuadTangent, MidTexIsTheQuadsOwnUvAverage) {
 const Scene scene = meshEverything();
 ASSERT_GE(scene.mesh.vertices.size(), 4U);
 for(std::size_t q = 0; q + 3 < scene.mesh.vertices.size(); q += 4) {
  float midU = 0.0f;
  float midV = 0.0f;
  for(std::size_t c = 0; c < 4; ++c) {
   midU += scene.mesh.vertices[q + c].u;
   midV += scene.mesh.vertices[q + c].v;
  }
  midU /= 4.0f;
  midV /= 4.0f;
  for(std::size_t c = 0; c < 4; ++c) {
   EXPECT_NEAR(scene.mesh.vertices[q + c].midU, midU, 1.0e-5f) << "quad " << q / 4 << " corner " << c;
   EXPECT_NEAR(scene.mesh.vertices[q + c].midV, midV, 1.0e-5f) << "quad " << q / 4 << " corner " << c;
  }
 }
}
} // namespace net::minecraft::test
