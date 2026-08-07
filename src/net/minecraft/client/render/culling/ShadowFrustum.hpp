#pragma once
#include <array>
#include <cstddef>
#include "net/minecraft/client/render/culling/PlaneSet.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render {
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/ShadowCullState.java
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/ShaderProperties.java
enum class ShadowCullState { Default,
                             Advanced,
                             SafeZone,
                             Distance };
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/frustum/BoxCuller.java
class BoxCuller {
 public:
 BoxCuller() = default;
 explicit BoxCuller(double maxDistance) : maxDistance_(maxDistance) {}
 void setPosition(double cameraX, double cameraY, double cameraZ) noexcept {
  minAllowedX_ = cameraX - maxDistance_;
  maxAllowedX_ = cameraX + maxDistance_;
  minAllowedY_ = cameraY - maxDistance_;
  maxAllowedY_ = cameraY + maxDistance_;
  minAllowedZ_ = cameraZ - maxDistance_;
  maxAllowedZ_ = cameraZ + maxDistance_;
 }
 [[nodiscard]] bool isCulled(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
     const noexcept {
  if(maxX < minAllowedX_ || minX > maxAllowedX_) return true;
  if(maxY < minAllowedY_ || minY > maxAllowedY_) return true;
  return maxZ < minAllowedZ_ || minZ > maxAllowedZ_;
 }
 [[nodiscard]] double maxDistance() const noexcept {
  return maxDistance_;
 }

 private:
 double maxDistance_ = 0.0;
 double minAllowedX_ = 0.0;
 double maxAllowedX_ = 0.0;
 double minAllowedY_ = 0.0;
 double maxAllowedY_ = 0.0;
 double minAllowedZ_ = 0.0;
 double maxAllowedZ_ = 0.0;
};
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/frustum/advanced/AdvancedShadowCullingFrustum.java
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/frustum/BoxCuller.java
class ShadowCullingFrustum {
 public:
 enum class Mode { NonCulling,
                   BoxCulling,
                   Advanced,
                   SafeZone };
 static constexpr std::size_t kMaxClippingPlanes = 13;
 void buildAdvanced(const float modelViewProjection[16], const float lightVectorFromOrigin[3]);
 void setMode(Mode mode) noexcept {
  mode_ = mode;
 }
 [[nodiscard]] Mode mode() const noexcept {
  return mode_;
 }
 void setBoxCuller(const BoxCuller& culler) noexcept {
  boxCuller_ = culler;
  hasBoxCuller_ = true;
 }
 void clearBoxCuller() noexcept {
  hasBoxCuller_ = false;
 }
 [[nodiscard]] bool hasBoxCuller() const noexcept {
  return hasBoxCuller_;
 }
 [[nodiscard]] double boxCullerDistance() const noexcept {
  return hasBoxCuller_ ? boxCuller_.maxDistance() : -1.0;
 }
 void setDistanceCuller(const BoxCuller& culler) noexcept {
  distanceCuller_ = culler;
  hasDistanceCuller_ = true;
 }
 void prepare(double cameraX, double cameraY, double cameraZ) noexcept;
 [[nodiscard]] bool isVisible(const net::minecraft::Box& box) const noexcept;
 [[nodiscard]] bool isVisible(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
     const noexcept;
 [[nodiscard]] std::size_t planeCount() const noexcept {
  return planes_.count();
 }

 private:
 void addPlane(const std::array<float, 4>& plane) noexcept;
 void addEdgePlane(const std::array<float, 4>& backPlane, const std::array<float, 4>& frontPlane) noexcept;
 // Same plane storage and same AABB test as the view Frustum; only the plane
 // count and how they are built differ. cornersVisible() used to be a second
 // hand-written copy of that test.
 PlaneSet<kMaxClippingPlanes> planes_{};
 float lightVector_[3] = {0.0f, 1.0f, 0.0f};
 Mode mode_ = Mode::NonCulling;
 BoxCuller boxCuller_{};
 BoxCuller distanceCuller_{};
 bool hasBoxCuller_ = false;
 bool hasDistanceCuller_ = false;
 double x_ = 0.0;
 double y_ = 0.0;
 double z_ = 0.0;
};
struct ShadowFrustumParams {
 ShadowCullState cullState = ShadowCullState::Default;
 bool packHasVoxelization = false;
 float halfPlaneLength = 160.0f; // shadowDistance
 float voxelDistance = 0.0f; // voxelDistance
 float renderMultiplier = -1.0f; // shadowDistanceRenderMul
 // options render distance, in blocks
 float renderDistanceBlocks = 0.0f;
 // When true, force box-only culling and skip the view-dependent "advanced" extrusion
 // (F6 shadow-acne A/B diagnostic).
 bool forceBoxCull = false;
};
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
[[nodiscard]] ShadowCullingFrustum createShadowFrustum(const ShadowFrustumParams& params,
                                                       const float modelViewProjection[16],
                                                       const float lightVectorFromOrigin[3]);
} // namespace net::minecraft::client::render
