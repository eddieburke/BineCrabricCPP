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
inline void directionToView(float x, float y, float z, const FrameRenderCamera& c, float out[3]) {
 out[0] = x * c.viewRightX + y * c.viewRightY + z * c.viewRightZ;
 out[1] = x * c.viewUpX + y * c.viewUpY + z * c.viewUpZ;
 out[2] = -(x * c.viewForwardX + y * c.viewForwardY + z * c.viewForwardZ);
}
inline void dirToView(float wx, float wy, float wz, const FrameRenderCamera& cam, float out[3]) {
 directionToView(wx, wy, wz, cam, out);
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
class RenderCameraState {
 public:
 static RenderCameraState& instance() noexcept;
 void setFrame(FrameRenderCamera camera) noexcept;
 [[nodiscard]] const FrameRenderCamera& frame() const noexcept {
  return frame_;
 }
 void clearFrame() noexcept;

 private:
 FrameRenderCamera frame_{};
};
} // namespace net::minecraft::client::render
