#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::client::render::buildShadowCelestialModelView;
using net::minecraft::client::render::buildShadowDirectionalModelView;
using net::minecraft::client::render::shadowAngleFromCelestial;
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
//
// The rotation columns are Iris' ShadowMatrices.Tests "model view at dawn" fixture
// verbatim. The TRANSLATION column deliberately differs from that fixture in z:
// Iris expects -100.4463119506836, we expect -0.4463119506836.
//
// Iris' fixture is stale. Their expectation was generated when
// createBaselineModelViewMatrix still did `translate(0, 0, -100)`; 26.1's version
// (read it — the else branch is three mulPose rotations and nothing else) no longer
// does, and NEAR = -100.05f already extends the ortho box 100 blocks behind the
// shadow camera, so re-adding the translate would double-count it. The stale number
// survived because ShadowMatrices.Tests is dead code that never runs: it is a private
// main(), and its test() helper prints "failed" on the equals() TRUE branch, so it
// could not have reported a mismatch even if something invoked it.
//
// Everything else here (R * snapOffset for the x and y rows) matches the fixture
// exactly, which is what pins the rotation chain and the grid snap.
TEST(ShadowCelestialModelView, MatchesIrisDawnFixture) {
 float m[16]{};
 buildShadowCelestialModelView(m, 0.03451777f, 0.0f, 2.0f, 0.646045982837677, 82.53274536132812,
                               -514.0264282226562);
 const float expected[16] = {
     0.21545040607452393f,
     5.820481518981069e-8f,
     0.9765146970748901f,
     0.0f,
     -0.9765147466795349f,
     1.2841844920785661e-8f,
     0.21545039117336273f,
     0.0f,
     0.0f,
     -0.9999999403953582f,
     5.960464477539063e-8f,
     0.0f,
     0.38002151250839233f,
     1.0264281034469604f,
     -0.4463119506836f,
     1.0f,
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
// Java: `target.mulPose(Axis.XP.rotationDegrees(sunPathRotation))` — the sun path
// rotation is applied POSITIVE about X, right after the sky angle. Every other test
// here passes 0, which is exactly why the sign could be inverted unnoticed: a flipped
// sun path tilts the shadow map away from the sunPosition/shadowLightPosition the pack
// lights with, and the two only disagree once a pack sets the directive.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
TEST(ShadowCelestialModelView, SunPathRotationIsPositiveAboutX) {
 constexpr float kSunPath = 30.0f;
 float m[16]{};
 // shadowAngle 0.25 → skyAngle 0, so the chain collapses to XP(90) * XP(sunPath).
 buildShadowCelestialModelView(m, 0.25f, kSunPath, 0.0f, 0.0, 0.0, 0.0);
 const float rad = (90.0f + kSunPath) * 3.14159265f / 180.0f;
 EXPECT_NEAR(m[0], 1.0f, 1e-5f);
 EXPECT_NEAR(m[5], std::cos(rad), 1e-5f);
 EXPECT_NEAR(m[6], std::sin(rad), 1e-5f);
 EXPECT_NEAR(m[9], -std::sin(rad), 1e-5f);
 EXPECT_NEAR(m[10], std::cos(rad), 1e-5f);
}
// The shadow angle fed to the model view is Java's getShadowAngle():
// getSunAngle(isDay())/360 where getSunAngle(sun) = (SUN_ANGLE + 90) mod 360 and
// isDay() = getSunAngle(sun) < 180. The moon angle is 180 degrees from the sun. The
// wrap is `c > 360` only, so an exact 360 does NOT wrap: at celestial 0.75 the sun
// angle is 1.0 (still "night"), and the shadow angle is the moon's (0.5).
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
TEST(ShadowCelestialModelView, ShadowAngleMatchesIrisCelestialUniforms) {
 // Day quadrants (isDay: getSunAngle(true) < 180): shadowAngle = (celestial + 0.25) mod 1.
 EXPECT_NEAR(shadowAngleFromCelestial(0.0f), 0.25f, 1e-6f);
 EXPECT_NEAR(shadowAngleFromCelestial(0.1f), 0.35f, 1e-6f);
 // Celestial 0.75 → SUN_ANGLE 270 + 90 = 360; 360 is NOT > 360 so it does not wrap,
 // isDay(360 < 180) is false, and the moon angle (90 + 90 = 180) /360 = 0.5.
 EXPECT_NEAR(shadowAngleFromCelestial(0.75f), 0.5f, 1e-6f);
 EXPECT_NEAR(shadowAngleFromCelestial(0.9f), 0.15f, 1e-6f);
 // Night quadrants (moon above the horizon): shadowAngle = (celestial - 0.25).
 EXPECT_NEAR(shadowAngleFromCelestial(0.3f), 0.05f, 1e-6f);
 EXPECT_NEAR(shadowAngleFromCelestial(0.5f), 0.25f, 1e-6f);
 EXPECT_NEAR(shadowAngleFromCelestial(0.6f), 0.35f, 1e-6f);
 // Continuous at the day/night boundaries.
 EXPECT_NEAR(shadowAngleFromCelestial(0.24f), 0.49f, 1e-6f);
 EXPECT_NEAR(shadowAngleFromCelestial(0.26f), 0.01f, 1e-6f);
}
// Night-time model view: shadowAngle 0.05 (celestial 0.3) → skyAngle 0.8 → Z rotation
// of -288 degrees. The matrix is Rx(90) * Rz(skyAngle * -360) * Rx(sunPathRotation)
// (the dawn fixture pins m[1]=m[5]=0, m[9]=-1), so the top-left 2x2 is
// [[cos, -sin], [0, 0]] and column 3 holds [sin, 0, cos].
TEST(ShadowCelestialModelView, NightSkyAngleRotation) {
 float m[16]{};
 buildShadowCelestialModelView(m, 0.05f, 0.0f, 0.0f, 0.0, 0.0, 0.0);
 const float skyAngle = 0.8f; // (0.05 + 0.75)
 const float theta = skyAngle * -360.0f * 3.14159265358979323846f / 180.0f;
 const float c = std::cos(theta);
 const float s = std::sin(theta);
 EXPECT_NEAR(m[0], c, 1e-4f);
 EXPECT_NEAR(m[4], -s, 1e-4f);
 EXPECT_NEAR(m[2], s, 1e-4f);
 EXPECT_NEAR(m[6], c, 1e-4f);
 EXPECT_NEAR(m[1], 0.0f, 1e-5f);
 EXPECT_NEAR(m[5], 0.0f, 1e-5f);
 EXPECT_NEAR(m[9], -1.0f, 1e-5f);
 EXPECT_NEAR(m[10], 0.0f, 1e-5f);
 EXPECT_NEAR(m[12], 0.0f, 1e-5f);
 EXPECT_NEAR(m[13], 0.0f, 1e-5f);
 EXPECT_NEAR(m[14], 0.0f, 1e-5f);
}
TEST(ShadowCelestialOrthoScale, HalfPlaneMatchesPearlProjScale) {
 // Pearl: shadow_proj_scale.x = 1/shadowDistance; our ortho m[0] = 1/orthoHalfWidth.
 using net::minecraft::client::render::buildCameraProjection;
 using net::minecraft::client::render::FrameRenderCamera;
 FrameRenderCamera cam{};
 cam.orthographic = true;
 cam.orthoHalfWidth = 160.0f;
 cam.orthoHalfHeight = 160.0f;
 cam.nearPlane = -227.0f;
 cam.farPlane = 227.0f;
 float proj[16]{};
 buildCameraProjection(proj, cam);
 EXPECT_NEAR(proj[0], 1.0f / 160.0f, 1e-6f);
 EXPECT_NEAR(proj[5], 1.0f / 160.0f, 1e-6f);
 EXPECT_NEAR(proj[10], -2.0f / (227.0f - (-227.0f)), 1e-6f);
}
// The pack-facing shadow light direction must agree with the orientation of the shadow
// map itself. Iris publishes the celestial positions through the fixed RY(-90) renderSky
// yaw (CelestialUniforms.getCelestialPosition: gbufferModelView * RY(-90) * RZ(spr) *
// RX(angle) * (0, 100, 0, w)); the light-registry direction is the beta renderSky frame
// BEFORE that yaw, so the yawed direction must land exactly on the shadow matrix's
// toward-light axis R^T * (0,0,1) = (m[2], m[6], m[10]) — sunPosition by day,
// moonPosition (= -sunPosition) by night. Before the yaw was applied, the two disagreed
// by up to ~87 degrees of azimuth, and the pack's slope-scaled normal bias + lit gate in
// lib/light/shadows.glsl (sample_shadow) smeared shadow boundaries into wedges on
// vertical faces.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
TEST(ShadowCelestialModelView, PackFacingLightDirectionMatchesMapOrientation) {
 using net::minecraft::client::render::applyRenderSkyYaw;
 using net::minecraft::client::render::buildShadowCelestialModelView;
 using net::minecraft::client::render::celestialSunAngle;
 for(int i = 0; i <= 24; ++i) {
  const float celestial = static_cast<float>(i) / 24.0f;
  // Light-registry sun direction: beta renderSky frame (0, cos 2pi*c, sin 2pi*c).
  const float angle = celestial * 6.28318530718f;
  const float registry[3] = {0.0f, std::cos(angle), std::sin(angle)};
  float yawed[3]{};
  applyRenderSkyYaw(registry, yawed);
  // Day/night split mirrors FrameData's shadowLightPosition selection
  // (sunAngleDegrees < 180 -> sun).
  const bool day = celestialSunAngle(celestial) < 0.5f;
  const float light[3] = {day ? yawed[0] : -yawed[0], day ? yawed[1] : -yawed[1],
                          day ? yawed[2] : -yawed[2]};
  // Shadow matrix toward-light axis: R^T * (0,0,1) = row 2 of the model view.
  float m[16]{};
  buildShadowCelestialModelView(m, shadowAngleFromCelestial(celestial), 0.0f, 0.0f, 0.0, 0.0, 0.0);
  const float axis[3] = {m[2], m[6], m[10]};
  const float dot = light[0] * axis[0] + light[1] * axis[1] + light[2] * axis[2];
  EXPECT_NEAR(dot, 1.0f, 1e-4f) << "celestial " << celestial;
 }
}
// The same invariant for the pack-facing uniform itself. FrameData publishes
// sunPosition/moonPosition/shadowLightPosition from celestialDirectionWorld, so that
// direction must land on the shadow map's toward-light axis for EVERY sunPathRotation,
// not just zero — a pack that sets one tilts the map and the celestial path together.
// Reading the celestial matrix' column 2 instead of column 1 put the two exactly
// perpendicular (dot 0) at every angle.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
TEST(ShadowCelestialModelView, CelestialStateMatchesMapOrientation) {
 using net::minecraft::client::render::CelestialState;
 using net::minecraft::client::render::makeCelestialState;
 for(const float sunPathRotation : {0.0f, 25.0f, -30.0f, 90.0f}) {
  for(int i = 0; i <= 24; ++i) {
   const float celestial = static_cast<float>(i) / 24.0f;
   const CelestialState state = makeCelestialState(celestial, sunPathRotation);
   // The shadow camera is built from the SAME struct (ShadowMapPass::makeShadowCamera),
   // so this pins the two together rather than hoping two derivations agree.
   float m[16]{};
   buildShadowCelestialModelView(m, state.shadowAngle, state.sunPathRotation, 0.0f, 0.0, 0.0, 0.0);
   const float axis[3] = {m[2], m[6], m[10]};
   const float* light = state.shadowLightDirectionWorld;
   const float dot = light[0] * axis[0] + light[1] * axis[1] + light[2] * axis[2];
   EXPECT_NEAR(dot, 1.0f, 1e-4f) << "celestial " << celestial << " sunPathRotation " << sunPathRotation;
   // moonDirectionWorld is the sun's antipode, and shadowLightDirectionWorld picks
   // between them on CelestialUniforms.isDay().
   const float* expected = state.day ? state.sunDirectionWorld : state.moonDirectionWorld;
   for(int c = 0; c < 3; ++c) {
    EXPECT_NEAR(light[c], expected[c], 1e-6f) << "component " << c << " celestial " << celestial;
   }
  }
 }
}
TEST(ShadowCelestialModelView, DirectionOverrideAlignsShadowAxis) {
 const float directions[][3] = {{0.2f, 0.9f, -0.3f}, {-0.8f, 0.1f, 0.5f}, {0.0f, 1.0f, 0.0f}};
 for(const auto& direction : directions) {
  float m[16]{};
  buildShadowDirectionalModelView(m, direction, 2.0f, 101.25, 64.0, -29.5);
  const float length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] +
                                 direction[2] * direction[2]);
  EXPECT_NEAR(m[2], direction[0] / length, 1e-6f);
  EXPECT_NEAR(m[6], direction[1] / length, 1e-6f);
  EXPECT_NEAR(m[10], direction[2] / length, 1e-6f);
 }
}
TEST(ShadowCelestialModelView, CelestialOverrideAnglesNormalizeAndRejectNonFiniteValues) {
 using net::minecraft::client::render::normalizeCelestialAngle;
 EXPECT_FLOAT_EQ(normalizeCelestialAngle(1.25f, 0.0f), 0.25f);
 EXPECT_FLOAT_EQ(normalizeCelestialAngle(-0.25f, 0.0f), 0.75f);
 EXPECT_FLOAT_EQ(normalizeCelestialAngle(std::numeric_limits<float>::infinity(), 0.4f), 0.4f);
}
}
} // namespace net::minecraft::test
