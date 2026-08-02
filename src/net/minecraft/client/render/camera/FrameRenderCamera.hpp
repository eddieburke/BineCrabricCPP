#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::entity {
class LivingEntity;
}
namespace net::minecraft::client::render {
struct FrameRenderCamera {
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double eyeX = 0.0;
 double eyeY = 0.0;
 double eyeZ = 0.0;
 float viewRightX = 1.0f;
 float viewRightY = 0.0f;
 float viewRightZ = 0.0f;
 float viewUpX = 0.0f;
 float viewUpY = 1.0f;
 float viewUpZ = 0.0f;
 float viewForwardX = 0.0f;
 float viewForwardY = 0.0f;
 float viewForwardZ = 1.0f;
 float projectionX = 1.0f;
 float projectionY = 1.0f;
 float yaw = 0.0f;
 float pitch = 0.0f;
 float roll = 0.0f;
 bool customView = false;
 bool hideFirstPersonHand = false;
 bool orthographic = false;
 float orthoHalfWidth = 1.0f;
 float orthoHalfHeight = 1.0f;
 float orthoNear = -1.0f;
 float orthoFar = 1.0f;
 float perspectiveNear = 0.05f;
 float perspectiveFar = 0.0f;
 bool shadowPass = false;
 bool shadowEntities = true;
 bool shadowPlayer = true;
 bool shadowTerrain = true;
 bool shadowTranslucent = true;
 bool shadowBlockEntities = true;
 bool shadowLightBlockEntities = true;
 bool skipAllRendering = false;
 float shadowEntityDistance = 0.0f;
 float frustumBypassDistance = 48.0f;
 float shadowRenderDistance = 0.0f;
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
 bool hasExplicitModelView = false;
 float explicitModelView[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};

// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
// Java createBaselineModelViewMatrix takes an additional end-flash branch in the End
// dimension when the pipeline supports it (ShadowMatrices.java:46-55); this port has
// no EndFlashState (beta 1.7.3 has no end flash), so only the celestial branch below
// applies.
inline void buildShadowCelestialModelView(float* out,
                                          float shadowAngle,
                                          float sunPathRotation,
                                          float shadowIntervalSize,
                                          double cameraX,
                                          double cameraY,
                                           double cameraZ) {
 net::minecraft::util::math::Matrix4f pose;
 // createBaselineModelViewMatrix
 float skyAngle = shadowAngle < 0.25f ? shadowAngle + 0.75f : shadowAngle - 0.25f;
 pose.rotate(90.0f, 1.0f, 0.0f, 0.0f);
 pose.rotate(skyAngle * -360.0f, 0.0f, 0.0f, 1.0f);
 pose.rotate(sunPathRotation, 1.0f, 0.0f, 0.0f);
 // snapModelViewToGrid
 if(std::abs(shadowIntervalSize) > 0.0f) {
  // Java: (float) cameraX % shadowIntervalSize
  float offsetX = std::fmod(static_cast<float>(cameraX), shadowIntervalSize);
  float offsetY = std::fmod(static_cast<float>(cameraY), shadowIntervalSize);
  float offsetZ = std::fmod(static_cast<float>(cameraZ), shadowIntervalSize);
  const float half = shadowIntervalSize * 0.5f;
  offsetX -= half;
  offsetY -= half;
  offsetZ -= half;
  pose.translate(offsetX, offsetY, offsetZ);
 }
 std::memcpy(out, pose.data(), sizeof(float) * 16);
}
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
inline void directionToView(float x, float y, float z, const FrameRenderCamera& c, float out[3]) {
 out[0] = x * c.viewRightX + y * c.viewRightY + z * c.viewRightZ;
 out[1] = x * c.viewUpX + y * c.viewUpY + z * c.viewUpZ;
 out[2] = -(x * c.viewForwardX + y * c.viewForwardY + z * c.viewForwardZ);
}
inline void buildCameraModelView(float* m, const FrameRenderCamera& c) {
 if(c.hasExplicitModelView) {
  std::memcpy(m, c.explicitModelView, sizeof(float) * 16);
  return;
 }
 std::fill(m, m + 16, 0.0f);
 m[0] = c.viewRightX;
 m[4] = c.viewRightY;
 m[8] = c.viewRightZ;
 m[1] = c.viewUpX;
 m[5] = c.viewUpY;
 m[9] = c.viewUpZ;
 m[2] = -c.viewForwardX;
 m[6] = -c.viewForwardY;
 m[10] = -c.viewForwardZ;
 m[15] = 1.0f;
}
inline void buildCameraModelViewInverse(float* m, const FrameRenderCamera& c) {
 std::fill(m, m + 16, 0.0f);
 m[0] = c.viewRightX;
 m[1] = c.viewRightY;
 m[2] = c.viewRightZ;
 m[4] = c.viewUpX;
 m[5] = c.viewUpY;
 m[6] = c.viewUpZ;
 m[8] = -c.viewForwardX;
 m[9] = -c.viewForwardY;
 m[10] = -c.viewForwardZ;
 m[15] = 1.0f;
}
inline void buildCameraProjection(float* m, const FrameRenderCamera& c, float farPlane) {
 std::fill(m, m + 16, 0.0f);
 if(c.orthographic) {
  const float w = std::max(c.orthoHalfWidth, 1e-3f);
  const float h = std::max(c.orthoHalfHeight, 1e-3f);
  const float nearZ = c.orthoNear;
  const float farZ = std::max(c.orthoFar, nearZ + 1e-3f);
  m[0] = 1.0f / w;
  m[5] = 1.0f / h;
  m[10] = -2.0f / (farZ - nearZ);
  m[14] = -(farZ + nearZ) / (farZ - nearZ);
  m[15] = 1.0f;
  return;
 }
 const float x = c.projectionX != 0.0f ? c.projectionX : 1.0f;
 const float y = c.projectionY != 0.0f ? c.projectionY : 1.0f;
 const float nearZ = std::max(0.001f, c.perspectiveNear);
 const float farZ = c.perspectiveFar > nearZ ? c.perspectiveFar : (farPlane > nearZ ? farPlane : nearZ + 1.0f);
 m[0] = x;
 m[5] = y;
 m[10] = -(farZ + nearZ) / (farZ - nearZ);
 m[11] = -1.0f;
 m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
}
inline void buildCameraProjectionInverse(float* m, const FrameRenderCamera& c, float farPlane) {
 const float x = std::abs(c.projectionX) > 1e-6f ? c.projectionX : 1.0f;
 const float y = std::abs(c.projectionY) > 1e-6f ? c.projectionY : 1.0f;
 const float nearZ = std::max(c.perspectiveNear, 1e-4f);
 const float farZ = c.perspectiveFar > nearZ ? c.perspectiveFar : (farPlane > nearZ ? farPlane : nearZ + 1e-3f);
 std::fill(m, m + 16, 0.0f);
 if(c.orthographic) {
  const float w = std::max(c.orthoHalfWidth, 1e-3f);
  const float h = std::max(c.orthoHalfHeight, 1e-3f);
  const float f = std::max(c.orthoFar, c.orthoNear + 1e-3f);
  m[0] = w;
  m[5] = h;
  m[10] = -(f - c.orthoNear) / 2.0f;
  m[14] = -(f + c.orthoNear) / 2.0f;
  m[15] = 1.0f;
  return;
 }
 m[0] = 1.0f / x;
 m[5] = 1.0f / y;
 m[11] = -(farZ - nearZ) / (2.0f * farZ * nearZ);
 m[14] = -1.0f;
 m[15] = (farZ + nearZ) / (2.0f * farZ * nearZ);
}
// Java CameraUniforms.CameraPositionTracker: keeps the shader camera position small by
// shifting the origin in 30000-block increments (X/Z only — Y is never shifted) so
// float precision survives large world coordinates; a 1000-block per-frame delta
// counts as a teleport and re-shifts immediately. previous* mirror the shifted and
// unshifted state before this frame's update, exactly like Java's update() order.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CameraUniforms.java
class CameraPositionTracker {
 public:
  static constexpr double kWalkRange = 30000.0;
  static constexpr double kTeleportRange = 1000.0;
  void update(double x, double y, double z) noexcept {
   for(int axis = 0; axis < 3; ++axis) {
    previous_[axis] = current_[axis];
    previousUnshifted_[axis] = currentUnshifted_[axis];
   }
   currentUnshifted_[0] = x;
   currentUnshifted_[1] = y;
   currentUnshifted_[2] = z;
   for(int axis = 0; axis < 3; ++axis) {
    current_[axis] = currentUnshifted_[axis] + shift_[axis];
   }
   updateShift();
  }
  [[nodiscard]] double current(int axis) const noexcept { return current_[axis]; }
  [[nodiscard]] double previous(int axis) const noexcept { return previous_[axis]; }
  [[nodiscard]] double currentUnshifted(int axis) const noexcept { return currentUnshifted_[axis]; }
  [[nodiscard]] double previousUnshifted(int axis) const noexcept { return previousUnshifted_[axis]; }

 private:
  static double getShift(double value, double prevValue) noexcept {
   if(std::abs(value) > kWalkRange || std::abs(value - prevValue) > kTeleportRange) {
    return -(value - std::fmod(value, kWalkRange));
   }
   return 0.0;
  }
  void updateShift() noexcept {
   const double dx = getShift(current_[0], previous_[0]);
   const double dz = getShift(current_[2], previous_[2]);
   if(dx != 0.0 || dz != 0.0) {
    shift_[0] += dx;
    current_[0] += dx;
    previous_[0] += dx;
    shift_[2] += dz;
    current_[2] += dz;
    previous_[2] += dz;
   }
  }
  double shift_[3]{};
  double current_[3]{};
  double previous_[3]{};
  double currentUnshifted_[3]{};
  double previousUnshifted_[3]{};
};
// Published per frame by GameRenderer; consumers (pipeline stages, entity/particle
// renderers, world renderers, Lua bindings) read the single iris camera frame instead
// of re-deriving camera state from the vanilla camera entity.
class RenderCameraState {
 public:
  static RenderCameraState& instance() noexcept {
   static RenderCameraState state;
   return state;
  }
  void setFrame(FrameRenderCamera camera) noexcept { frame_ = camera; }
  [[nodiscard]] const FrameRenderCamera& frame() const noexcept {
   return frame_;
  }
  void clearFrame() noexcept { frame_ = {}; }

 private:
  FrameRenderCamera frame_{};
};
} // namespace net::minecraft::client::render
