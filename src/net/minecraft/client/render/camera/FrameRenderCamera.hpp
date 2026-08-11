#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include "net/minecraft/client/render/celestial/CelestialState.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::entity {
class LivingEntity;
}
namespace net::minecraft::client::render {
class ShadowCullingFrustum;
struct FrameRenderCamera {
 // Camera ENTITY position (interpolated feet position), not the camera.
 double x = 0.0;
 double y = 0.0;
 double z = 0.0;
 double eyeX = 0.0;
 double eyeY = 0.0;
 double eyeZ = 0.0;
 // Clean camera rotation basis (from the camera transform alone, no bobbing).
 float viewRightX = 1.0f;
 float viewRightY = 0.0f;
 float viewRightZ = 0.0f;
 float viewUpX = 0.0f;
 float viewUpY = 1.0f;
 float viewUpZ = 0.0f;
 float viewForwardX = 0.0f;
 float viewForwardY = 0.0f;
 float viewForwardZ = 1.0f;
 // `modelView * bob`", MixinModelViewBobbing.java:101). Vanilla puts these effects in
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/mixin/MixinModelViewBobbing.java
 bool hasBobModelView = false;
 float bobModelView[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
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
 float zoomScale = 1.0f;
 float zoomOffsetX = 0.0f;
 float zoomOffsetY = 0.0f;
 // Scales the projection's z row. The hand pass uses this so the held item cannot
 // clip into world geometry; everything else leaves it at 1.
 float depthScale = 1.0f;
 // Depth size the shadow FBO actually got (see ShadowTargets::ensure). glPolygonOffset
 // units are multiples of the smallest resolvable depth increment, so a bias stated in
 // blocks cannot be converted without it.
 int shadowDepthBits = 24;
 float nearPlane = 0.05f;
 // The PROJECTION far plane only (Beta's EntityRenderer: farPlaneDistance * 2).
 // Never the pack's `far` uniform — that one is renderDistanceBlocks below.
 float farPlane = 256.0f;
 // The pack's `far`: getEffectiveRenderDistance() * 16, i.e. sectionCoverageBlocks().
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CameraUniforms.java
 float renderDistanceBlocks = 256.0f;
 bool shadowPass = false;
 bool shadowEntities = true;
 bool shadowPlayer = true;
 bool shadowTerrain = true;
 bool shadowTranslucent = true;
 bool shadowBlockEntities = true;
 bool shadowLightBlockEntities = true;
 bool skipAllRendering = false;
 float frustumBypassDistance = 48.0f;
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
 const ShadowCullingFrustum* shadowTerrainFrustum = nullptr;
 const ShadowCullingFrustum* shadowEntityFrustum = nullptr;
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
 bool hasExplicitModelView = false;
 float explicitModelView[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
// dimension when the pipeline supports it (ShadowMatrices.java:46-55); this port has
inline void buildShadowCelestialModelView(float* out,
                                          float shadowAngle,
                                          float sunPathRotation,
                                          float shadowIntervalSize,
                                          double cameraX,
                                          double cameraY,
                                          double cameraZ) {
 float skyAngle = shadowAngle < 0.25f ? shadowAngle + 0.75f : shadowAngle - 0.25f;
 net::minecraft::util::math::Matrix4f pose;
 pose.rotate(90.0f, 1.0f, 0.0f, 0.0f);
 pose.rotate(skyAngle * -360.0f, 0.0f, 0.0f, 1.0f);
 pose.rotate(sunPathRotation, 1.0f, 0.0f, 0.0f);
 if(std::abs(shadowIntervalSize) > 0.0f) {
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
inline void buildShadowDirectionalModelView(float* out,
                                            const float direction[3],
                                            float shadowIntervalSize,
                                            double cameraX,
                                            double cameraY,
                                            double cameraZ) {
 const float length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] +
                                direction[2] * direction[2]);
 const float light[3] = {direction[0] / length, direction[1] / length, direction[2] / length};
 const float reference[3] = {0.0f, std::abs(light[1]) < 0.99f ? 1.0f : 0.0f,
                             std::abs(light[1]) < 0.99f ? 0.0f : 1.0f};
 float right[3] = {reference[1] * light[2] - reference[2] * light[1],
                   reference[2] * light[0] - reference[0] * light[2],
                   reference[0] * light[1] - reference[1] * light[0]};
 const float rightLength = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
 for(float& value : right) {
  value /= rightLength;
 }
 const float up[3] = {light[1] * right[2] - light[2] * right[1],
                      light[2] * right[0] - light[0] * right[2],
                      light[0] * right[1] - light[1] * right[0]};
 net::minecraft::util::math::Matrix4f pose;
 pose.m[0] = right[0];
 pose.m[4] = right[1];
 pose.m[8] = right[2];
 pose.m[1] = up[0];
 pose.m[5] = up[1];
 pose.m[9] = up[2];
 pose.m[2] = light[0];
 pose.m[6] = light[1];
 pose.m[10] = light[2];
 if(std::abs(shadowIntervalSize) > 0.0f) {
  float offsetX = std::fmod(static_cast<float>(cameraX), shadowIntervalSize);
  float offsetY = std::fmod(static_cast<float>(cameraY), shadowIntervalSize);
  float offsetZ = std::fmod(static_cast<float>(cameraZ), shadowIntervalSize);
  const float half = shadowIntervalSize * 0.5f;
  pose.translate(offsetX - half, offsetY - half, offsetZ - half);
 }
 std::memcpy(out, pose.data(), sizeof(float) * 16);
}
// Clean camera rotation, no translation: `MV_camera` on its own.
inline void buildCameraViewRotation(float* m, const FrameRenderCamera& c) {
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
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/mixin/MixinModelViewBobbing.java
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CameraUniforms.java
inline void buildCameraModelView(float* m, const FrameRenderCamera& c) {
 net::minecraft::util::math::Matrix4f cameraMatrix;
 if(c.hasExplicitModelView) {
  cameraMatrix.set(c.explicitModelView);
 } else {
  float rotation[16]{};
  buildCameraViewRotation(rotation, c);
  cameraMatrix.set(rotation);
 }
 if(!c.hasBobModelView) {
  std::memcpy(m, cameraMatrix.data(), sizeof(float) * 16);
  return;
 }
 net::minecraft::util::math::Matrix4f bobbed;
 bobbed.set(c.bobModelView);
 bobbed.multiply(cameraMatrix); // bob * MV_camera (Java mulLocal)
 std::memcpy(m, bobbed.data(), sizeof(float) * 16);
}
inline void buildCameraModelViewInverse(float* m, const FrameRenderCamera& c) {
 // The bob carries a nausea scale, so this is a full inverse, not a transpose.
 float modelView[16]{};
 buildCameraModelView(modelView, c);
 net::minecraft::util::math::Matrix4f inverse;
 inverse.set(modelView);
 inverse.invert();
 std::memcpy(m, inverse.data(), sizeof(float) * 16);
}
// Java's celestial/up uniforms transform w=0 vectors by gbufferModelView
// (CelestialUniforms.getCelestialPosition/getUpPosition), so a view-space direction
// picks up the bob's rotation and scale but none of its translation.
inline void directionToView(float x, float y, float z, const FrameRenderCamera& c, float out[3]) {
 float modelView[16]{};
 buildCameraModelView(modelView, c);
 net::minecraft::util::math::Matrix4f mv;
 mv.set(modelView);
 mv.transformDirection(x, y, z, out[0], out[1], out[2]);
}
inline void buildCameraProjection(float* m, const FrameRenderCamera& c) {
 net::minecraft::util::math::Matrix4f proj;
 if(c.orthographic) {
  // Symmetric ortho box; Matrix4f::ortho is the same formula this used to
  // hand-write element by element.
  const float w = std::max(c.orthoHalfWidth, 1e-3f);
  const float h = std::max(c.orthoHalfHeight, 1e-3f);
  // The ortho near is legitimately negative (the shadow box straddles the light
  // plane), so it is NOT clamped away from zero the way the perspective near is.
  const float nearZ = c.nearPlane;
  const float farZ = std::max(c.farPlane, nearZ + 1e-3f);
  proj.ortho(-w, w, -h, h, nearZ, farZ);
 } else {
  // Not Matrix4f::perspective: that takes fov/aspect, while the camera carries
  // the already-resolved focal terms (a pack can override them outright), so the
  // fov would have to be reconstructed just to be turned back into these.
  std::fill(proj.m, proj.m + 16, 0.0f);
  const float x = c.projectionX != 0.0f ? c.projectionX : 1.0f;
  const float y = c.projectionY != 0.0f ? c.projectionY : 1.0f;
  const float nearZ = std::max(c.nearPlane, 1e-4f);
  const float farZ = std::max(c.farPlane, nearZ + 1e-3f);
  proj.m[0] = x;
  proj.m[5] = y;
  proj.m[10] = -(farZ + nearZ) / (farZ - nearZ);
  proj.m[11] = -1.0f;
  proj.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
 }
 // Row 2 (the z row) scaled in place: vanilla's hand pass shrinks the depth range
 // so the held item cannot clip into the world.
 if(c.depthScale != 1.0f) {
  for(int column = 0; column < 4; ++column) {
   proj.m[column * 4 + 2] *= c.depthScale;
  }
 }
 // Zoom is a post-projection NDC transform, so it LEFT-multiplies: `T * S * P`,
 // matching the order the raster site used to build by hand.
 if(c.zoomScale != 1.0f || c.zoomOffsetX != 0.0f || c.zoomOffsetY != 0.0f) {
  net::minecraft::util::math::Matrix4f zoom;
  zoom.translate(c.zoomOffsetX, c.zoomOffsetY, 0.0f);
  zoom.scale(c.zoomScale, c.zoomScale, 1.0f);
  zoom.multiply(proj);
  std::memcpy(m, zoom.data(), sizeof(float) * 16);
  return;
 }
 std::memcpy(m, proj.data(), sizeof(float) * 16);
}
inline void buildCameraProjectionInverse(float* m, const FrameRenderCamera& c) {
 float projection[16]{};
 buildCameraProjection(projection, c);
 net::minecraft::util::math::Matrix4f inverse;
 inverse.set(projection);
 inverse.invert();
 std::memcpy(m, inverse.data(), sizeof(float) * 16);
}
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
} // namespace net::minecraft::client::render
