#pragma once
#include <cmath>
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::client::render {
// Java CelestialUniforms.getSunAngle(sun)/360: (angle + 90) mod 360 wrapped with a
// single `c > 360` check, so an exact 1.0 does NOT wrap (celestial 0.75 stays 1.0).
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
inline float celestialSunAngle(float celestialAngle01) {
 const float wrapped = celestialAngle01 + 0.25f;
 return wrapped > 1.0f ? wrapped - 1.0f : wrapped;
}
// Java: shadowAngle uniform = getShadowAngle() = getSunAngle(isDay())/360, where
// isDay() = getSunAngle(true) < 180 (sunAngle < 0.5). At night the moon angle is
// used, which is 180 degrees from the sun ((sunAngle + 0.5) mod 1), with the same
// >-only wrap — so at the celestial 0.75 wrap the shadow angle is 0.5 exactly like
// Java (360 does not wrap, isDay is false, the moon sits at 180 degrees).
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
inline float shadowAngleFromCelestial(float celestialAngle) {
 const float sunAngle = celestialSunAngle(celestialAngle);
 if(sunAngle < 0.5f) {
  return sunAngle;
 }
 const float moonAngle = celestialAngle < 0.5f ? celestialAngle + 0.5f : celestialAngle - 0.5f;
 return celestialSunAngle(moonAngle);
}
// Iris publishes the celestial positions through the fixed -90 degree renderSky yaw
// (`celestial.rotate(Axis.YP.rotationDegrees(-90.0F))`, CelestialUniforms.java:154-158)
// BEFORE the sunPathRotation/angle rotations. The beta renderSky direction
// (0, cos 2pi*c, sin 2pi*c) is that pre-yaw frame, so rotating it by RY(-90) yields
// (worldX, worldY, worldZ) -> (-worldZ, worldY, worldX), landing on the same axis the
// shadow map is oriented by (ShadowMatrices.createBaselineModelViewMatrix light axis =
// R^T * (0,0,1)).
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
inline void applyRenderSkyYaw(const float in[3], float out[3]) {
 out[0] = -in[2];
 out[1] = in[1];
 out[2] = in[0];
}
// getCelestialPositionInWorldSpace: RY(-90) * RZ(sunPathRotation) * RX(angle) applied to
// (0, 100, 0). The body rides the matrix' +Y axis — column 1, not column 2.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
inline void celestialSkyMatrix(float celestialAngle, float sunPathRotation,
                               net::minecraft::util::math::Matrix4f& out) {
 const float sunAngle = celestialSunAngle(celestialAngle);
 const float skyAngle = sunAngle < 0.25f ? sunAngle + 0.75f : sunAngle - 0.25f;
 out.identity();
 out.rotate(-90.0f, 0.0f, 1.0f, 0.0f);
 out.rotate(sunPathRotation, 0.0f, 0.0f, 1.0f);
 out.rotate(skyAngle * 360.0f, 1.0f, 0.0f, 0.0f);
}
inline void celestialDirectionWorld(float celestialAngle, float sunPathRotation, float out[3]) {
 net::minecraft::util::math::Matrix4f celestialMat;
 celestialSkyMatrix(celestialAngle, sunPathRotation, celestialMat);
 out[0] = celestialMat.m[4];
 out[1] = celestialMat.m[5];
 out[2] = celestialMat.m[6];
}
// The one answer to "where is the sun" for a frame. Built once per frame by
// GameRenderer::updateSunLight and published through RenderCore; SunLight,
// SkyUniforms, WorldLightUniforms, the pack uniforms in FrameData and the shadow
// camera in ShadowMapPass are all READERS of this. They each used to re-derive the
// celestial angle themselves, which is how SkyDome drifted onto a hardcoded 25 degree
// sunPathRotation and the wrong matrix column while the shadow map used another.
struct CelestialState {
 float celestialAngle = 0.0f;
 float sunPathRotation = 0.0f;
 // Iris sunAngle / shadowAngle uniforms, both 0..1.
 float sunAngle = 0.25f;
 float shadowAngle = 0.25f;
 int moonPhase = -1;
 bool day = true;
 float sunDirectionWorld[3] = {0.0f, 1.0f, 0.0f};
 float moonDirectionWorld[3] = {0.0f, -1.0f, 0.0f};
 // shadowLightPosition's world-space direction: the sun by day, the moon by night.
 // This must stay parallel to the shadow map's own toward-light axis
 // (buildShadowCelestialModelView row 2) — see tests/shadow_celestial_modelview_test.cpp.
 float shadowLightDirectionWorld[3] = {0.0f, 1.0f, 0.0f};
 bool directionOverride = false;
};
inline float normalizeCelestialAngle(float value, float fallback) {
 if(!std::isfinite(value)) return fallback;
 value = std::fmod(value, 1.0f);
 return value < 0.0f ? value + 1.0f : value;
}
inline CelestialState makeCelestialState(float celestialAngle, float sunPathRotation) {
 CelestialState state;
 state.celestialAngle = celestialAngle;
 state.sunPathRotation = sunPathRotation;
 state.sunAngle = celestialSunAngle(celestialAngle);
 state.shadowAngle = shadowAngleFromCelestial(celestialAngle);
 // Java CelestialUniforms.isDay(): getSunAngle(true) < 180.
 state.day = state.sunAngle < 0.5f;
 celestialDirectionWorld(celestialAngle, sunPathRotation, state.sunDirectionWorld);
 for(int i = 0; i < 3; ++i) {
  state.moonDirectionWorld[i] = -state.sunDirectionWorld[i];
  state.shadowLightDirectionWorld[i] =
      state.day ? state.sunDirectionWorld[i] : state.moonDirectionWorld[i];
 }
 return state;
}
} // namespace net::minecraft::client::render
