#include <gtest/gtest.h>
#include <cmath>
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::test {
namespace {
using client::render::Tessellator;
using client::render::TessellatorMesh;
using client::render::TessellatorVertex;
void unpackNormal(std::int32_t packed, float& x, float& y, float& z) {
 x = static_cast<float>(static_cast<std::int8_t>(packed & 0xFF)) / 127.0f;
 y = static_cast<float>(static_cast<std::int8_t>((packed >> 8) & 0xFF)) / 127.0f;
 z = static_cast<float>(static_cast<std::int8_t>((packed >> 16) & 0xFF)) / 127.0f;
}
// Resets the thread-local pose so tests don't bleed state into each other.
class NormalPackingTest : public ::testing::Test {
 protected:
 void SetUp() override {
  client::render::core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
 }
 void TearDown() override {
  client::render::core::setDrawPose(net::minecraft::util::math::Matrix4f::identityMatrix());
 }
};
} // namespace
// Every axis-aligned block face normal must survive the int8 pack/unpack
// round trip exactly, since lighting and bumpmap shaders read this value
// directly as vaNormal.
TEST_F(NormalPackingTest, AxisAlignedNormalsRoundTripExactly) {
 const float axisNormals[6][3] = {
     {1.0f, 0.0f, 0.0f},
     {-1.0f, 0.0f, 0.0f},
     {0.0f, 1.0f, 0.0f},
     {0.0f, -1.0f, 0.0f},
     {0.0f, 0.0f, 1.0f},
     {0.0f, 0.0f, -1.0f},
 };
 for(const auto& n : axisNormals) {
  Tessellator::INSTANCE.startQuads();
  Tessellator::INSTANCE.normal(n[0], n[1], n[2]);
  Tessellator::INSTANCE.vertex(0.0, 0.0, 0.0);
  Tessellator::INSTANCE.vertex(1.0, 0.0, 0.0);
  Tessellator::INSTANCE.vertex(1.0, 1.0, 0.0);
  Tessellator::INSTANCE.vertex(0.0, 1.0, 0.0);
  TessellatorMesh mesh = Tessellator::INSTANCE.takeMesh();
  ASSERT_TRUE(mesh.hasNormals);
  // The quad is expanded to two triangles (kTriangleMode), so 4 corners in
  // means 6 vertices out — and every one of them must carry the same normal.
  ASSERT_EQ(mesh.vertexCount(), 6U);
  for(std::size_t v = 0; v < mesh.vertexCount(); ++v) {
   float ux = 0.0f, uy = 0.0f, uz = 0.0f;
   unpackNormal(mesh.vertices[v].normal, ux, uy, uz);
   EXPECT_NEAR(ux, n[0], 1.0f / 127.0f) << "vertex " << v;
   EXPECT_NEAR(uy, n[1], 1.0f / 127.0f) << "vertex " << v;
   EXPECT_NEAR(uz, n[2], 1.0f / 127.0f) << "vertex " << v;
  }
 }
}
// A -1.0 component packs to int8 -127 (x*127.0f truncated), not -128, so the
// unpack must never produce a magnitude greater than 1.0 in either direction.
TEST_F(NormalPackingTest, NormalComponentsNeverExceedUnitMagnitude) {
 Tessellator::INSTANCE.startQuads();
 Tessellator::INSTANCE.normal(-1.0f, -1.0f, -1.0f);
 Tessellator::INSTANCE.vertex(0.0, 0.0, 0.0);
 Tessellator::INSTANCE.vertex(1.0, 0.0, 0.0);
 Tessellator::INSTANCE.vertex(1.0, 1.0, 0.0);
 Tessellator::INSTANCE.vertex(0.0, 1.0, 0.0);
 TessellatorMesh mesh = Tessellator::INSTANCE.takeMesh();
 float ux = 0.0f, uy = 0.0f, uz = 0.0f;
 unpackNormal(mesh.vertices[0].normal, ux, uy, uz);
 EXPECT_GE(ux, -1.0f);
 EXPECT_GE(uy, -1.0f);
 EXPECT_GE(uz, -1.0f);
}
// A pose that only rotates must re-normalize the transformed normal to unit
// length; if the rotation matrix carries any scale, unnormalized normals
// wash out or blow out specular/bumpmap terms in the shader.
TEST_F(NormalPackingTest, RotatedPoseKeepsNormalUnitLength) {
 net::minecraft::util::math::Matrix4f pose = net::minecraft::util::math::Matrix4f::identityMatrix();
 pose.rotate(37.0f, 0.0f, 1.0f, 0.0f);
 client::render::core::setDrawPose(pose);
 Tessellator::INSTANCE.startQuads();
 Tessellator::INSTANCE.normal(0.0f, 0.0f, 1.0f);
 Tessellator::INSTANCE.vertex(0.0, 0.0, 0.0);
 Tessellator::INSTANCE.vertex(1.0, 0.0, 0.0);
 Tessellator::INSTANCE.vertex(1.0, 1.0, 0.0);
 Tessellator::INSTANCE.vertex(0.0, 1.0, 0.0);
 TessellatorMesh mesh = Tessellator::INSTANCE.takeMesh();
 float ux = 0.0f, uy = 0.0f, uz = 0.0f;
 unpackNormal(mesh.vertices[0].normal, ux, uy, uz);
 const float length = std::sqrt(ux * ux + uy * uy + uz * uz);
 EXPECT_NEAR(length, 1.0f, 0.05f);
 // rotating +Z about Y by 37 degrees should not leave it pointing at +Z.
 EXPECT_GT(std::fabs(ux), 0.1f);
}
// A pure-translation pose (no rotation) must leave the packed normal
// untouched rather than routing it through the rotate-and-renormalize path,
// which would needlessly perturb it via floating-point rounding.
TEST_F(NormalPackingTest, TranslationOnlyPoseDoesNotPerturbNormal) {
 net::minecraft::util::math::Matrix4f pose = net::minecraft::util::math::Matrix4f::identityMatrix();
 pose.translate(5.0f, -3.0f, 12.0f);
 client::render::core::setDrawPose(pose);
 Tessellator::INSTANCE.startQuads();
 Tessellator::INSTANCE.normal(0.0f, 1.0f, 0.0f);
 Tessellator::INSTANCE.vertex(0.0, 0.0, 0.0);
 Tessellator::INSTANCE.vertex(1.0, 0.0, 0.0);
 Tessellator::INSTANCE.vertex(1.0, 1.0, 0.0);
 Tessellator::INSTANCE.vertex(0.0, 1.0, 0.0);
 TessellatorMesh mesh = Tessellator::INSTANCE.takeMesh();
 float ux = 0.0f, uy = 0.0f, uz = 0.0f;
 unpackNormal(mesh.vertices[0].normal, ux, uy, uz);
 EXPECT_NEAR(ux, 0.0f, 1.0f / 127.0f);
 EXPECT_NEAR(uy, 1.0f, 1.0f / 127.0f);
 EXPECT_NEAR(uz, 0.0f, 1.0f / 127.0f);
}
} // namespace net::minecraft::test
