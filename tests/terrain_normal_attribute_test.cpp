// Does terrain geometry carry a correct per-face normal?
//
// Two separate things live here, and they are easy to confuse:
//
//  1. BufferBuilder::kPrototype seeds every vertex with 0x00007F00 — packed
//     (0,1,0). That is the FALLBACK for producers that never call normal(), and
//     it is deliberate: a zero normal is NaN after the normalize() every Iris
//     gbuffer vertex stage applies.
//
//  2. The block mesher does NOT rely on that fallback. Every face emitter feeds
//     its face direction through emitBlockVertex, which calls
//     Tessellator::normal() before each vertex.
//
// These tests pin (2), because a regression there is invisible — the fallback
// would quietly stand in and every surface would light as sky-facing.
#include <gtest/gtest.h>
#include <cmath>
#include <cstdint>
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::test {
namespace {
using client::render::Tessellator;
using client::render::TessellatorMesh;
using client::render::block::emitBlockVertex;
constexpr std::int32_t kPackedUp = 0x00007F00;
struct Normal {
 float x = 0.0f;
 float y = 0.0f;
 float z = 0.0f;
};
Normal unpack(std::int32_t packed) {
 return {static_cast<float>(static_cast<std::int8_t>(packed & 0xFF)) / 127.0f,
         static_cast<float>(static_cast<std::int8_t>((packed >> 8) & 0xFF)) / 127.0f,
         static_cast<float>(static_cast<std::int8_t>((packed >> 16) & 0xFF)) / 127.0f};
}
// One quad emitted the way every BlockFaceRenderer face emits it.
TessellatorMesh faceQuad(float nx, float ny, float nz) {
 client::render::core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
 Tessellator::INSTANCE.startQuads();
 emitBlockVertex(Tessellator::INSTANCE, nx, ny, nz, 0.0, 0.0, 0.0, 0.0, 0.0);
 emitBlockVertex(Tessellator::INSTANCE, nx, ny, nz, 1.0, 0.0, 0.0, 1.0, 0.0);
 emitBlockVertex(Tessellator::INSTANCE, nx, ny, nz, 1.0, 1.0, 0.0, 1.0, 1.0);
 emitBlockVertex(Tessellator::INSTANCE, nx, ny, nz, 0.0, 1.0, 0.0, 0.0, 1.0);
 return Tessellator::INSTANCE.takeMesh();
}
// The same quad with NO normal() call anywhere — the path every renderer that
// emits through Tessellator::vertex directly takes (crops, torches, ladders,
// rails, redstone dust, repeaters, levers, fire, beds, pistons).
//
// The render stage is set because the face-normal recomputation is gated on
// Java's `ImmediateState.isRenderingLevel`: outside level rendering the
// producer's normal stands, exactly as in Java.
TessellatorMesh bareQuad(const Normal& a, const Normal& b, const Normal& c, const Normal& d) {
 client::render::core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
 const client::render::core::RenderStageScope stage(client::render::core::RenderStage::TerrainSolid);
 Tessellator::INSTANCE.startQuads();
 Tessellator::INSTANCE.texture(0.0, 0.0);
 for(const Normal& corner : {a, b, c, d}) {
  Tessellator::INSTANCE.vertex(corner.x, corner.y, corner.z);
 }
 return Tessellator::INSTANCE.takeMesh();
}
// A quad in the plane of the given face normal, with u running along the
// face's first in-plane axis and v along the second — the layout
// BlockFaceRenderer produces.
TessellatorMesh orientedFaceQuad(const Normal& n) {
 client::render::core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
 float ux = 0.0f, uy = 0.0f, uz = 0.0f;
 float vx = 0.0f, vy = 0.0f, vz = 0.0f;
 if(std::fabs(n.y) > 0.5f) { // top/bottom: u along X, v along Z
  ux = 1.0f;
  vz = 1.0f;
 } else if(std::fabs(n.z) > 0.5f) { // east/west: u along X, v along Y
  ux = 1.0f;
  vy = 1.0f;
 } else { // north/south: u along Z, v along Y
  uz = 1.0f;
  vy = 1.0f;
 }
 const auto corner = [&](float a, float b, double u, double v) {
  emitBlockVertex(Tessellator::INSTANCE, n.x, n.y, n.z, ux * a + vx * b, uy * a + vy * b,
                  uz * a + vz * b, u, v);
 };
 Tessellator::INSTANCE.startQuads();
 corner(0.0f, 0.0f, 0.0, 0.0);
 corner(1.0f, 0.0f, 1.0, 0.0);
 corner(1.0f, 1.0f, 1.0, 1.0);
 corner(0.0f, 1.0f, 0.0, 1.0);
 return Tessellator::INSTANCE.takeMesh();
}
} // namespace
// The six cube face directions, exactly as BlockFaceRenderer passes them
// (bottom -Y, top +Y, east -Z, west +Z, north -X, south +X). Every vertex of a
// face must carry that face's own direction — not the prototype's +Y.
TEST(TerrainNormalAttribute, EveryCubeFaceCarriesItsOwnDirection) {
 const Normal faces[] = {{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
 for(const Normal& face : faces) {
  const TessellatorMesh mesh = faceQuad(face.x, face.y, face.z);
  ASSERT_GT(mesh.vertexCount(), 0U);
  ASSERT_TRUE(mesh.hasNormals);
  for(std::size_t v = 0; v < mesh.vertexCount(); ++v) {
   const Normal n = unpack(mesh.vertices[v].normal);
   EXPECT_NEAR(n.x, face.x, 1.0f / 127.0f) << "face (" << face.x << "," << face.y << "," << face.z << ")";
   EXPECT_NEAR(n.y, face.y, 1.0f / 127.0f) << "face (" << face.x << "," << face.y << "," << face.z << ")";
   EXPECT_NEAR(n.z, face.z, 1.0f / 127.0f) << "face (" << face.x << "," << face.y << "," << face.z << ")";
  }
 }
}
// The regression that would be silent: a side face falling back to the
// prototype's +Y. If the mesher ever stops calling normal(), this catches it —
// a wall would otherwise light as though it faced the sky.
TEST(TerrainNormalAttribute, SideFacesDoNotFallBackToThePrototypeUpNormal) {
 for(const Normal& side : {Normal{0.0f, 0.0f, -1.0f}, Normal{1.0f, 0.0f, 0.0f}}) {
  const TessellatorMesh mesh = faceQuad(side.x, side.y, side.z);
  ASSERT_GT(mesh.vertexCount(), 0U);
  EXPECT_NE(mesh.vertices[0].normal, kPackedUp)
      << "side face reported the prototype up normal; the mesher stopped emitting per-face normals";
 }
}
// Face normals are unit length, so normalize() in the gbuffer stage is a no-op
// rather than a rescale.
TEST(TerrainNormalAttribute, FaceNormalsAreUnitLength) {
 const TessellatorMesh mesh = faceQuad(0.0f, 0.0f, -1.0f);
 ASSERT_GT(mesh.vertexCount(), 0U);
 const Normal n = unpack(mesh.vertices[0].normal);
 EXPECT_NEAR(std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z), 1.0f, 1.0f / 127.0f);
}
// A producer that never calls normal() gets the GEOMETRIC normal of the quad,
// derived from its own winding at finishQuad time. That is not a nicety: crops,
// torches, ladders, rails, redstone dust, repeaters, levers, fire, beds and
// pistons all emit through Tessellator::vertex directly, so before the
// derivation every one of their quads — including the vertical ones — shipped
// the prototype's +Y and lit as though it faced the sky.
TEST(TerrainNormalAttribute, UnsetNormalIsDerivedFromTheWinding) {
 // A quad in the XY plane; cross(v1 - v0, v2 - v0) is +Z.
 const TessellatorMesh mesh = bareQuad({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
                                       {0.0f, 1.0f, 0.0f});
 ASSERT_GT(mesh.vertexCount(), 0U);
 for(std::size_t v = 0; v < mesh.vertexCount(); ++v) {
  const Normal n = unpack(mesh.vertices[v].normal);
  EXPECT_NEAR(n.x, 0.0f, 1.0f / 127.0f);
  EXPECT_NEAR(n.y, 0.0f, 1.0f / 127.0f);
  EXPECT_NEAR(n.z, 1.0f, 1.0f / 127.0f);
 }
 EXPECT_NE(mesh.vertices[0].normal, kPackedUp) << "kept the prototype instead of deriving";
}
// The recomputation has to agree with the hand-written normals for the same
// winding, or the cube faces (which call normal()) and everything else (which
// does not) would describe the same surface as facing opposite ways.
TEST(TerrainNormalAttribute, DerivedNormalMatchesTheHandWrittenOneForTheSameWinding) {
 const Normal corners[] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
 // Same four corners, once through emitBlockVertex with an explicit +Z and once
 // bare. Both must land on +Z.
 client::render::core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
 Tessellator::INSTANCE.startQuads();
 for(const Normal& c : corners) {
  emitBlockVertex(Tessellator::INSTANCE, 0.0f, 0.0f, 1.0f, c.x, c.y, c.z, 0.0, 0.0);
 }
 const TessellatorMesh explicitMesh = Tessellator::INSTANCE.takeMesh();
 const TessellatorMesh derivedMesh = bareQuad(corners[0], corners[1], corners[2], corners[3]);
 ASSERT_GT(explicitMesh.vertexCount(), 0U);
 ASSERT_EQ(explicitMesh.vertexCount(), derivedMesh.vertexCount());
 for(std::size_t v = 0; v < explicitMesh.vertexCount(); ++v) {
  EXPECT_EQ(explicitMesh.vertices[v].normal, derivedMesh.vertices[v].normal) << "vertex " << v;
 }
}
// THE bug this was written for, and the one a per-part "did anyone call
// normal()" flag does not fix: a chunk mesh emits cube faces and crop quads into
// the SAME tessellator part. Once a cube face has called normal(), a flag-based
// fallback is off for the rest of the part AND hasNormals_ is true, so the crop
// quads behind it inherit the last cube face's normal — a vertical quad claiming
// to face whichever way the neighbouring block's face did.
//
// Java has no such flag: it recomputes every quad's face normal while the level
// is rendering. So the crop quad must come out facing +Z no matter what was
// emitted before it in the same part.
TEST(TerrainNormalAttribute, ABareQuadAfterAnExplicitOneStillGetsItsOwnNormal) {
 client::render::core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
 const client::render::core::RenderStageScope stage(client::render::core::RenderStage::TerrainSolid);
 Tessellator::INSTANCE.startQuads();
 // A "cube face": explicit +Y, lying in the XZ plane.
 emitBlockVertex(Tessellator::INSTANCE, 0.0f, 1.0f, 0.0f, 0.0, 0.0, 0.0, 0.0, 0.0);
 emitBlockVertex(Tessellator::INSTANCE, 0.0f, 1.0f, 0.0f, 0.0, 0.0, 1.0, 0.0, 1.0);
 emitBlockVertex(Tessellator::INSTANCE, 0.0f, 1.0f, 0.0f, 1.0, 0.0, 1.0, 1.0, 1.0);
 emitBlockVertex(Tessellator::INSTANCE, 0.0f, 1.0f, 0.0f, 1.0, 0.0, 0.0, 1.0, 0.0);
 // A "crop": vertical, no normal() call, in the same part.
 Tessellator::INSTANCE.texture(0.0, 0.0);
 Tessellator::INSTANCE.vertex(0.0, 0.0, 0.0);
 Tessellator::INSTANCE.vertex(1.0, 0.0, 0.0);
 Tessellator::INSTANCE.vertex(1.0, 1.0, 0.0);
 Tessellator::INSTANCE.vertex(0.0, 1.0, 0.0);
 const TessellatorMesh mesh = Tessellator::INSTANCE.takeMesh();
 ASSERT_GE(mesh.vertexCount(), 8U);
 // Last quad's vertices: vertical, so +Z — not the +Y the cube face left behind.
 for(std::size_t v = mesh.vertexCount() - 4; v < mesh.vertexCount(); ++v) {
  const Normal n = unpack(mesh.vertices[v].normal);
  EXPECT_NEAR(n.z, 1.0f, 1.0f / 127.0f)
      << "vertex " << v << " inherited the preceding face's normal instead of its own";
  EXPECT_NEAR(n.y, 0.0f, 1.0f / 127.0f) << "vertex " << v;
 }
}
// Outside level rendering Java leaves the producer's normal alone
// (`recalculateNormal = ImmediateState.isRenderingLevel`). GUI and item-batching
// geometry must not be silently reshaded.
TEST(TerrainNormalAttribute, NormalsAreNotRecomputedOutsideLevelRendering) {
 client::render::core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
 const client::render::core::RenderStageScope stage(client::render::core::RenderStage::None);
 Tessellator::INSTANCE.startQuads();
 // Explicit +Y on a quad whose winding says +Z; the explicit value must survive.
 emitBlockVertex(Tessellator::INSTANCE, 0.0f, 1.0f, 0.0f, 0.0, 0.0, 0.0, 0.0, 0.0);
 emitBlockVertex(Tessellator::INSTANCE, 0.0f, 1.0f, 0.0f, 1.0, 0.0, 0.0, 1.0, 0.0);
 emitBlockVertex(Tessellator::INSTANCE, 0.0f, 1.0f, 0.0f, 1.0, 1.0, 0.0, 1.0, 1.0);
 emitBlockVertex(Tessellator::INSTANCE, 0.0f, 1.0f, 0.0f, 0.0, 1.0, 0.0, 0.0, 1.0);
 const TessellatorMesh mesh = Tessellator::INSTANCE.takeMesh();
 ASSERT_GT(mesh.vertexCount(), 0U);
 EXPECT_EQ(mesh.vertices[0].normal, kPackedUp);
}
// Reversing the winding must reverse the normal — that is what lets crops and
// torches emit a front and a back quad and have each face its own way.
TEST(TerrainNormalAttribute, ReversedWindingDerivesTheOppositeNormal) {
 const Normal a{0.0f, 0.0f, 0.0f}, b{1.0f, 0.0f, 0.0f}, c{1.0f, 1.0f, 0.0f}, d{0.0f, 1.0f, 0.0f};
 const Normal front = unpack(bareQuad(a, b, c, d).vertices[0].normal);
 const Normal back = unpack(bareQuad(d, c, b, a).vertices[0].normal);
 EXPECT_NEAR(front.z, 1.0f, 1.0f / 127.0f);
 EXPECT_NEAR(back.z, -1.0f, 1.0f / 127.0f);
}
// The prototype is still the fallback where there is no winding to read: a
// degenerate (zero-area) quad has no geometric normal, and a zero vector would
// be NaN after the normalize() every Iris gbuffer vertex stage applies. Pinned
// so nobody "simplifies" it back to zero.
TEST(TerrainNormalAttribute, DegenerateQuadKeepsTheUnitUpPrototype) {
 const Normal origin{0.0f, 0.0f, 0.0f};
 const TessellatorMesh mesh = bareQuad(origin, origin, origin, origin);
 ASSERT_GT(mesh.vertexCount(), 0U);
 EXPECT_EQ(mesh.vertices[0].normal, kPackedUp);
}
// ---- Tangents: what normal maps actually ride on -------------------------
//
// RenderPearl (prog/lit_deferred.fsh) builds
//   mat3 tbn = mat3(tangent, cross(tangent, normal) * handedness, normal)
// and multiplies the sampled normal-map vector by it. If the tangent is
// parallel to the normal that cross product is the zero vector, the basis
// collapses, and the normal map contributes garbage. A constant +X tangent —
// which is what the pose-basis version produced — is parallel to the normal on
// exactly the +/-X faces.
// The tangent must never be parallel to its own face normal, on ANY face.
TEST(TerrainTangent, TangentIsNeverParallelToTheFaceNormal) {
 const Normal faces[] = {{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
 for(const Normal& face : faces) {
  const TessellatorMesh mesh = orientedFaceQuad(face);
  ASSERT_GT(mesh.vertexCount(), 0U);
  const auto& vertex = mesh.vertices[0];
  const float tx = static_cast<float>(vertex.tangent[0]) / 32767.0f;
  const float ty = static_cast<float>(vertex.tangent[1]) / 32767.0f;
  const float tz = static_cast<float>(vertex.tangent[2]) / 32767.0f;
  // cross(tangent, normal) is the bitangent column of the TBN; if it is zero
  // the matrix is singular and the normal map is dead on this face.
  const float cx = ty * face.z - tz * face.y;
  const float cy = tz * face.x - tx * face.z;
  const float cz = tx * face.y - ty * face.x;
  const float bitangentLength = std::sqrt(cx * cx + cy * cy + cz * cz);
  EXPECT_GT(bitangentLength, 0.1f)
      << "face (" << face.x << "," << face.y << "," << face.z
      << "): tangent is parallel to the normal, TBN is degenerate, normal maps are dead here";
 }
}
// Tangents are unit length so the pack's normalize() is a no-op.
TEST(TerrainTangent, TangentIsUnitLength) {
 const Normal faces[] = {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
 for(const Normal& face : faces) {
  const TessellatorMesh mesh = orientedFaceQuad(face);
  ASSERT_GT(mesh.vertexCount(), 0U);
  const auto& vertex = mesh.vertices[0];
  const float tx = static_cast<float>(vertex.tangent[0]) / 32767.0f;
  const float ty = static_cast<float>(vertex.tangent[1]) / 32767.0f;
  const float tz = static_cast<float>(vertex.tangent[2]) / 32767.0f;
  EXPECT_NEAR(std::sqrt(tx * tx + ty * ty + tz * tz), 1.0f, 0.02f);
 }
}
// Handedness must be one of the two unit signs the pack reads out of .w.
TEST(TerrainTangent, HandednessIsASignNotGarbage) {
 const TessellatorMesh mesh = orientedFaceQuad({0.0f, 1.0f, 0.0f});
 ASSERT_GT(mesh.vertexCount(), 0U);
 const std::int16_t w = mesh.vertices[0].tangent[3];
 EXPECT_TRUE(w == 32767 || w == -32767) << "handedness was " << w;
}
// All three vertices of a triangle share one tangent — a per-vertex tangent
// that varied across a flat face would shear the normal map across it.
TEST(TerrainTangent, AllVerticesOfAFaceShareOneTangent) {
 const TessellatorMesh mesh = orientedFaceQuad({0.0f, 0.0f, -1.0f});
 ASSERT_GE(mesh.vertexCount(), 3U);
 for(std::size_t v = 1; v < 3; ++v) {
  EXPECT_EQ(mesh.vertices[v].tangent[0], mesh.vertices[0].tangent[0]);
  EXPECT_EQ(mesh.vertices[v].tangent[1], mesh.vertices[0].tangent[1]);
  EXPECT_EQ(mesh.vertices[v].tangent[2], mesh.vertices[0].tangent[2]);
  EXPECT_EQ(mesh.vertices[v].tangent[3], mesh.vertices[0].tangent[3]);
 }
}
// The tangent should follow +U: for a top face laid out with u increasing along
// +X, the tangent is +X.
TEST(TerrainTangent, TangentFollowsTheUAxis) {
 const TessellatorMesh mesh = orientedFaceQuad({0.0f, 1.0f, 0.0f});
 ASSERT_GT(mesh.vertexCount(), 0U);
 const float tx = static_cast<float>(mesh.vertices[0].tangent[0]) / 32767.0f;
 EXPECT_NEAR(std::fabs(tx), 1.0f, 0.02f) << "top face u runs along X, so the tangent should too";
}
} // namespace net::minecraft::test
