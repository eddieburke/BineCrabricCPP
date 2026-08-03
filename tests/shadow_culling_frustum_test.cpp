#include <gtest/gtest.h>
#include <cmath>
#include "net/minecraft/client/render/culling/ShadowFrustum.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::client::render::createShadowFrustum;
using net::minecraft::client::render::ShadowCullingFrustum;
using net::minecraft::client::render::ShadowCullState;
using net::minecraft::client::render::ShadowFrustumParams;
// Column-major perspective projection, matching buildCameraProjection.
void perspective(float* m, float fovDegrees, float aspect, float nearZ, float farZ) {
 for(int i = 0; i < 16; ++i) m[i] = 0.0f;
 const float f = 1.0f / std::tan(fovDegrees * 3.14159265f / 360.0f);
 m[0] = f / aspect;
 m[5] = f;
 m[10] = (farZ + nearZ) / (nearZ - farZ);
 m[11] = -1.0f;
 m[14] = 2.0f * farZ * nearZ / (nearZ - farZ);
}
// Camera looking along -Z (identity rotation), geometry relative to the camera.
void identityModelView(float* m) {
 for(int i = 0; i < 16; ++i) m[i] = 0.0f;
 m[0] = m[5] = m[10] = m[15] = 1.0f;
}
void multiply(const float* a, const float* b, float* out) {
 for(int column = 0; column < 4; ++column) {
  for(int row = 0; row < 4; ++row) {
   float sum = 0.0f;
   for(int k = 0; k < 4; ++k) sum += a[k * 4 + row] * b[column * 4 + k];
   out[column * 4 + row] = sum;
  }
 }
}
ShadowFrustumParams advancedParams() {
 ShadowFrustumParams params;
 params.cullState = ShadowCullState::Advanced;
 params.halfPlaneLength = 160.0f;
 params.renderMultiplier = -1.0f; // falls back to the user shadow distance
 params.renderDistanceBlocks = 256.0f;
 return params;
}
ShadowCullingFrustum buildLookingNorth(const float lightVector[3]) {
 float projection[16]{};
 float modelView[16]{};
 float projView[16]{};
 perspective(projection, 70.0f, 16.0f / 9.0f, 0.05f, 256.0f);
 identityModelView(modelView);
 multiply(projection, modelView, projView);
 ShadowCullingFrustum frustum = createShadowFrustum(advancedParams(), projView, lightVector);
 frustum.prepare(0.0, 64.0, 0.0);
 return frustum;
}
net::minecraft::Box blockAt(double x, double y, double z, double size = 16.0) {
 return net::minecraft::Box(x, y, z, x + size, y + size, z + size);
}
// The whole point of the extruded frustum: a caster BEHIND the player, on the side the
// light comes from, still shadows what the player can see, so it must survive culling
// even though it is nowhere near the player's view frustum. Culling the shadow pass
// with the camera's own frustum (or the shadow ortho box) drops these, which is what
// makes shadows pop and splotch as the camera turns.
TEST(ShadowCullingFrustum, KeepsCastersBehindThePlayerOnTheLightSide) {
 // Light high in the sky and behind the camera (camera looks toward -Z).
 const float light[3] = {0.0f, 0.7071f, 0.7071f};
 const ShadowCullingFrustum frustum = buildLookingNorth(light);
 EXPECT_EQ(frustum.mode(), ShadowCullingFrustum::Mode::Advanced);
 // Directly in view.
 EXPECT_TRUE(frustum.isVisible(blockAt(-8.0, 56.0, -64.0)));
 // Behind the camera, between the light and the visible region: casts into view.
 // Light rays run (0,-1,-1)/sqrt(2), so a caster reaches view depth u at height
 // (y - z) - u; landing inside a 70 degree frustum needs 0.3u <= y - z <= 1.7u. This box
 // is camera-relative y-z in [32, 64], so its shadow enters the view volume.
 EXPECT_TRUE(frustum.isVisible(blockAt(-8.0, 128.0, 16.0)));
}
// The complement: geometry on the far side of the view volume from the light cannot
// cast onto anything visible, so Java's back-plane test drops it.
TEST(ShadowCullingFrustum, CullsGeometryOppositeTheLight) {
 const float light[3] = {0.0f, 0.7071f, 0.7071f};
 const ShadowCullingFrustum frustum = buildLookingNorth(light);
 // Far below the world, away from the light, well outside the view cone.
 EXPECT_FALSE(frustum.isVisible(blockAt(-8.0, -400.0, -600.0)));
}
// Java: (DEFAULT && packHasVoxelization) or DISTANCE -> distance-only culling, and no
// culling at all when the distance would reach past the render distance.
TEST(ShadowCullingFrustum, VoxelizingPackFallsBackToDistanceCulling) {
 float projView[16]{};
 float projection[16]{};
 float modelView[16]{};
 perspective(projection, 70.0f, 16.0f / 9.0f, 0.05f, 256.0f);
 identityModelView(modelView);
 multiply(projection, modelView, projView);
 const float light[3] = {0.0f, 1.0f, 0.0f};
 ShadowFrustumParams params = advancedParams();
 params.cullState = ShadowCullState::Default;
 params.packHasVoxelization = true;
 params.renderMultiplier = 0.5f; // 160 * 0.5 = 80 blocks
 ShadowCullingFrustum boxed = createShadowFrustum(params, projView, light);
 boxed.prepare(0.0, 64.0, 0.0);
 EXPECT_EQ(boxed.mode(), ShadowCullingFrustum::Mode::BoxCulling);
 EXPECT_TRUE(boxed.isVisible(blockAt(-8.0, 56.0, 40.0)));
 EXPECT_FALSE(boxed.isVisible(blockAt(-8.0, 56.0, 200.0)));
 // 160 * 2 = 320 blocks > 256 render distance -> nothing is culled.
 params.renderMultiplier = 2.0f;
 ShadowCullingFrustum uncapped = createShadowFrustum(params, projView, light);
 uncapped.prepare(0.0, 64.0, 0.0);
 EXPECT_EQ(uncapped.mode(), ShadowCullingFrustum::Mode::NonCulling);
 EXPECT_TRUE(uncapped.isVisible(blockAt(-8.0, 56.0, 2000.0)));
}
// Java SafeZoneCullingFrustum: anything inside the voxel box is kept regardless of the
// extruded frustum, and the half-plane box is a hard outer cut.
TEST(ShadowCullingFrustum, SafeZoneKeepsTheVoxelBoxAndCutsPastTheHalfPlane) {
 float projView[16]{};
 float projection[16]{};
 float modelView[16]{};
 perspective(projection, 70.0f, 16.0f / 9.0f, 0.05f, 256.0f);
 identityModelView(modelView);
 multiply(projection, modelView, projView);
 const float light[3] = {0.0f, 0.7071f, 0.7071f};
 ShadowFrustumParams params = advancedParams();
 params.cullState = ShadowCullState::SafeZone;
 params.voxelDistance = 48.0f;
 params.halfPlaneLength = 160.0f;
 params.renderMultiplier = -1.0f; // safe zone forces the multiplier to 1
 ShadowCullingFrustum frustum = createShadowFrustum(params, projView, light);
 frustum.prepare(0.0, 64.0, 0.0);
 EXPECT_EQ(frustum.mode(), ShadowCullingFrustum::Mode::SafeZone);
 // Inside the 48-block voxel box but well outside the view cone and away from the
 // light: kept anyway.
 EXPECT_TRUE(frustum.isVisible(blockAt(-8.0, 40.0, 16.0)));
 // Past the 160-block half plane: dropped no matter what.
 EXPECT_FALSE(frustum.isVisible(blockAt(-8.0, 56.0, -400.0)));
}
} // namespace
} // namespace net::minecraft::test
