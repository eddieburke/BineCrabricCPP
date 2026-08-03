#pragma once
#include <array>
#include <cstddef>
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render {
// Java `shadow.culling`: false -> DISTANCE, true -> ADVANCED, reversed/safe_zone ->
// SAFE_ZONE, unset -> DEFAULT (advanced unless the pack voxelizes).
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/ShadowCullState.java
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/properties/ShaderProperties.java
enum class ShadowCullState { Default, Advanced, SafeZone, Distance };
// Java BoxCuller: an axis-aligned cube of `maxDistance` blocks around the player camera.
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
 [[nodiscard]] bool isCulled(const net::minecraft::Box& box) const noexcept {
  return isCulled(box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
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
// The shadow pass frustum. Iris never culls the shadow pass against the shadow map's
// own ortho frustum: it derives a tightly-fitted volume from the PLAYER's view frustum
// extruded toward the shadow light (L. Spiro's algorithm), on the assumption that
// something behind you cannot cast a direct shadow onto what you can see. Culling
// against the shadow ortho box instead drops casters that are outside the player's
// frustum but still shadow it, which shows up as shadows that appear/vanish/splotch as
// the camera turns.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/frustum/advanced/AdvancedShadowCullingFrustum.java
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/frustum/BoxCuller.java
class ShadowCullingFrustum {
 public:
 // NonCulling / BoxCulling are Java's fallback frusta; Advanced and SafeZone are the
 // extruded-player-frustum forms (SafeZone additionally passes anything inside the
 // voxel box straight through).
 enum class Mode { NonCulling, BoxCulling, Advanced, SafeZone };
 static constexpr std::size_t kMaxClippingPlanes = 13;
 // `modelViewProjection` is the player's gbufferProjection * gbufferModelView (column
 // major, geometry relative to the camera position); `lightVectorFromOrigin` is the
 // normalized WORLD-space direction toward the shadow light.
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
 void setDistanceCuller(const BoxCuller& culler) noexcept {
  distanceCuller_ = culler;
  hasDistanceCuller_ = true;
 }
 // Centres the frustum on the player camera, exactly like Java's
 // `frustum.prepare(cameraX, cameraY, cameraZ)`.
 void prepare(double cameraX, double cameraY, double cameraZ) noexcept;
 [[nodiscard]] bool isVisible(const net::minecraft::Box& box) const noexcept;
 [[nodiscard]] bool isVisible(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
     const noexcept;
 [[nodiscard]] std::size_t planeCount() const noexcept {
  return planeCount_;
 }
 [[nodiscard]] const std::array<float, 4>& plane(std::size_t index) const noexcept {
  return planes_[index];
 }

 private:
 [[nodiscard]] bool cornersVisible(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
     const noexcept;
 void addPlane(const std::array<float, 4>& plane) noexcept;
 void addEdgePlane(const std::array<float, 4>& backPlane, const std::array<float, 4>& frontPlane) noexcept;
 std::array<std::array<float, 4>, kMaxClippingPlanes> planes_{};
 std::size_t planeCount_ = 0;
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
// Everything Java's ShadowRenderer.createShadowFrustum reads out of the pack
// directives, plus the two engine-side values it takes from the game options.
struct ShadowFrustumParams {
 ShadowCullState cullState = ShadowCullState::Default;
 bool packHasVoxelization = false;
 float halfPlaneLength = 160.0f;    // shadowDistance
 float voxelDistance = 0.0f;        // voxelDistance
 float renderMultiplier = -1.0f;    // shadowDistanceRenderMul
 float renderDistanceBlocks = 0.0f; // options render distance, in blocks
 // Java falls back to the user's Iris shadow distance option when the pack leaves
 // shadowDistanceRenderMul negative (IrisVideoSettings.shadowDistance, 32 chunks).
 int userShadowDistanceChunks = 32;
};
// Java ShadowRenderer.createShadowFrustum. `modelViewProjection` is the player's
// gbufferProjection * gbufferModelView; `lightVectorFromOrigin` is the normalized
// world-space direction toward the shadow light
// (CelestialUniforms.getShadowLightPositionInWorldSpace).
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
[[nodiscard]] ShadowCullingFrustum createShadowFrustum(const ShadowFrustumParams& params,
                                                       const float modelViewProjection[16],
                                                       const float lightVectorFromOrigin[3]);
} // namespace net::minecraft::client::render
