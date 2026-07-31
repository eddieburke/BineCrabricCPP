#include <gtest/gtest.h>
#include <cmath>
#include "net/minecraft/client/render/FrameRenderCamera.hpp"

namespace net::minecraft::test {
namespace {
using net::minecraft::client::render::buildShadowCelestialModelView;

// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
TEST(ShadowCelestialModelView, MatchesIrisDawnFixture) {
 float m[16]{};
 buildShadowCelestialModelView(m, 0.03451777f, 0.0f, 2.0f, 0.646045982837677, 82.53274536132812,
                               -514.0264282226562);
 // Column-major expected from Iris ShadowMatrices.Tests (dawn).
 const float expected[16] = {
     0.21545040607452393f,  5.820481518981069e-8f, 0.9765146970748901f,  0.0f,
     -0.9765147466795349f,  1.2841844920785661e-8f, 0.21545039117336273f, 0.0f,
     0.0f,                 -0.9999999403953582f,   5.960464477539063e-8f, 0.0f,
     0.38002151250839233f,  1.0264281034469604f,   -100.4463119506836f,   1.0f,
 };
 for(int i = 0; i < 16; ++i) {
  EXPECT_NEAR(m[i], expected[i], 5e-4f) << "index " << i;
 }
}

TEST(ShadowCelestialModelView, QuarterShadowAngleIsXp90) {
 float m[16]{};
 // shadowAngle 0.25 → skyAngle 0 → only XP(90).
 buildShadowCelestialModelView(m, 0.25f, 0.0f, 0.0f, 100.0, 64.0, -200.0);
 EXPECT_NEAR(m[12], 0.0f, 1e-5f);
 EXPECT_NEAR(m[13], 0.0f, 1e-5f);
 EXPECT_NEAR(m[14], 0.0f, 1e-5f);
 EXPECT_NEAR(m[15], 1.0f, 1e-5f);
 EXPECT_NEAR(m[0], 1.0f, 1e-5f);
 EXPECT_NEAR(m[5], 0.0f, 1e-5f);
 EXPECT_NEAR(m[6], 1.0f, 1e-5f);
 EXPECT_NEAR(m[9], -1.0f, 1e-5f);
 EXPECT_NEAR(m[10], 0.0f, 1e-5f);
}

TEST(ShadowCelestialOrthoScale, HalfPlaneMatchesPearlProjScale) {
 // Pearl: shadow_proj_scale.x = 1/shadowDistance; our ortho m[0] = 1/orthoHalfWidth.
 using net::minecraft::client::render::FrameRenderCamera;
 using net::minecraft::client::render::buildCameraProjection;
 FrameRenderCamera cam{};
 cam.orthographic = true;
 cam.orthoHalfWidth = 160.0f;
 cam.orthoHalfHeight = 160.0f;
 cam.orthoNear = -227.0f;
 cam.orthoFar = 227.0f;
 float proj[16]{};
 buildCameraProjection(proj, cam, 256.0f);
 EXPECT_NEAR(proj[0], 1.0f / 160.0f, 1e-6f);
 EXPECT_NEAR(proj[5], 1.0f / 160.0f, 1e-6f);
 EXPECT_NEAR(proj[10], -2.0f / (227.0f - (-227.0f)), 1e-6f);
}
} // namespace
} // namespace net::minecraft::test
