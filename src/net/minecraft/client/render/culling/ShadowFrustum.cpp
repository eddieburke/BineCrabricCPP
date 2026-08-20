#include "net/minecraft/client/render/culling/ShadowFrustum.hpp"
#include <cmath>
namespace net::minecraft::client::render {
namespace {
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/frustum/advanced/BaseClippingPlanes.java
std::array<float, 4> clippingPlane(const float m[16], float cornerX, float cornerY, float cornerZ) {
 std::array<float, 4> plane{};
 for(int row = 0; row < 4; ++row) {
  plane[static_cast<std::size_t>(row)] = m[row * 4 + 0] * cornerX + m[row * 4 + 1] * cornerY +
                                         m[row * 4 + 2] * cornerZ + m[row * 4 + 3];
 }
 const float length = std::sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2] +
                                plane[3] * plane[3]);
 if(length > 0.0f) {
  for(float& component : plane) component /= length;
 }
 return plane;
}
std::array<float, 3> cross(const float a[3], const float b[3]) {
 return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/frustum/advanced/NeighboringPlaneSet.java
constexpr int kNeighboringPlanes[3][4] = {
    {2, 3, 4, 5}, // +/-X
    {0, 1, 4, 5}, // +/-Y
    {0, 1, 2, 3}, // +/-Z
};
} // namespace
void ShadowCullingFrustum::addPlane(const std::array<float, 4>& plane) noexcept {
 planes_.add(plane);
}
void ShadowCullingFrustum::addEdgePlane(const std::array<float, 4>& backPlane,
                                        const std::array<float, 4>& frontPlane) noexcept {
 const float backNormal[3] = {backPlane[0], backPlane[1], backPlane[2]};
 const float frontNormal[3] = {frontPlane[0], frontPlane[1], frontPlane[2]};
 const std::array<float, 3> intersection = cross(backNormal, frontNormal);
 const std::array<float, 3> edgeNormal = cross(intersection.data(), lightVector_);
 const float intersectionLengthSq =
     intersection[0] * intersection[0] + intersection[1] * intersection[1] + intersection[2] * intersection[2];
 if(intersectionLengthSq <= 0.0f) {
  return;
 }
 // https://stackoverflow.com/a/32410473, as used by Java.
 std::array<float, 3> ixb = cross(intersection.data(), backNormal);
 const std::array<float, 3> fxi = cross(frontNormal, intersection.data());
 std::array<float, 3> point{};
 for(std::size_t i = 0; i < 3; ++i) {
  point[i] = (ixb[i] * -frontPlane[3] + fxi[i] * -backPlane[3]) / intersectionLengthSq;
 }
 const float d = edgeNormal[0] * point[0] + edgeNormal[1] * point[1] + edgeNormal[2] * point[2];
 addPlane({edgeNormal[0], edgeNormal[1], edgeNormal[2], -d});
}
void ShadowCullingFrustum::buildAdvanced(const float modelViewProjection[16],
                                         const float lightVectorFromOrigin[3]) {
 planes_.clear();
 lightVector_[0] = lightVectorFromOrigin[0];
 lightVector_[1] = lightVectorFromOrigin[1];
 lightVector_[2] = lightVectorFromOrigin[2];
 const std::array<std::array<float, 4>, 6> base = {
     clippingPlane(modelViewProjection, -1.0f, 0.0f, 0.0f),
     clippingPlane(modelViewProjection, 1.0f, 0.0f, 0.0f),
     clippingPlane(modelViewProjection, 0.0f, -1.0f, 0.0f),
     clippingPlane(modelViewProjection, 0.0f, 1.0f, 0.0f),
     clippingPlane(modelViewProjection, 0.0f, 0.0f, -1.0f),
     clippingPlane(modelViewProjection, 0.0f, 0.0f, 1.0f),
 };
 bool isBack[6]{};
 for(std::size_t index = 0; index < base.size(); ++index) {
  const float dot = base[index][0] * lightVector_[0] + base[index][1] * lightVector_[1] +
                    base[index][2] * lightVector_[2];
  isBack[index] = dot > 0.0f;
  if(dot >= 0.0f) {
   addPlane(base[index]);
  }
 }
 for(std::size_t index = 0; index < base.size(); ++index) {
  if(!isBack[index]) {
   continue;
  }
  for(const int neighbor : kNeighboringPlanes[index >> 1]) {
   if(!isBack[static_cast<std::size_t>(neighbor)]) {
    addEdgePlane(base[index], base[static_cast<std::size_t>(neighbor)]);
   }
  }
 }
}
void ShadowCullingFrustum::prepare(double cameraX, double cameraY, double cameraZ) noexcept {
 boxCuller_.setPosition(cameraX, cameraY, cameraZ);
 distanceCuller_.setPosition(cameraX, cameraY, cameraZ);
 x_ = cameraX;
 y_ = cameraY;
 z_ = cameraZ;
}
bool ShadowCullingFrustum::isVisible(const net::minecraft::Box& box) const noexcept {
 return isVisible(box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
}
bool ShadowCullingFrustum::isVisible(double minX,
                                     double minY,
                                     double minZ,
                                     double maxX,
                                     double maxY,
                                     double maxZ) const noexcept {
 switch(mode_) {
 case Mode::NonCulling:
  return true;
 case Mode::BoxCulling:
  return !boxCuller_.isCulled(minX, minY, minZ, maxX, maxY, maxZ);
 case Mode::SafeZone:
  if(distanceCuller_.isCulled(minX, minY, minZ, maxX, maxY, maxZ)) {
   return false;
  }
  if(!boxCuller_.isCulled(minX, minY, minZ, maxX, maxY, maxZ)) {
   return true;
  }
  break;
 case Mode::Advanced:
  if(hasBoxCuller_ && boxCuller_.isCulled(minX, minY, minZ, maxX, maxY, maxZ)) {
   return false;
  }
  break;
 }
 return planes_.intersectsAabb(static_cast<float>(minX - x_), static_cast<float>(minY - y_),
                               static_cast<float>(minZ - z_), static_cast<float>(maxX - x_),
                               static_cast<float>(maxY - y_), static_cast<float>(maxZ - z_));
}
ShadowCullingFrustum createShadowFrustum(const ShadowFrustumParams& params,
                                         const float modelViewProjection[16],
                                         const float lightVectorFromOrigin[3]) {
 ShadowCullingFrustum frustum;
 if((params.cullState == ShadowCullState::Default && params.packHasVoxelization) ||
    params.cullState == ShadowCullState::Distance) {
  const double distance = static_cast<double>(params.halfPlaneLength) * params.renderMultiplier;
  if(distance <= 0.0 || distance > static_cast<double>(params.renderDistanceBlocks)) {
   frustum.setMode(ShadowCullingFrustum::Mode::NonCulling);
   return frustum;
  }
  frustum.setMode(ShadowCullingFrustum::Mode::BoxCulling);
  frustum.setBoxCuller(BoxCuller(distance));
  return frustum;
 }
 const bool hasSafeZone = params.cullState == ShadowCullState::SafeZone;
 float renderMultiplier = params.renderMultiplier;
 if(hasSafeZone && renderMultiplier < 0.0f) {
  renderMultiplier = 1.0f;
 }
 double distance = static_cast<double>(hasSafeZone ? params.voxelDistance : params.halfPlaneLength) *
                   renderMultiplier;
 if(renderMultiplier < 0.0f) {
  distance = static_cast<double>(params.renderDistanceBlocks);
 }
 if(distance <= 0.0) {
  frustum.setMode(ShadowCullingFrustum::Mode::NonCulling);
  return frustum;
 }
 frustum.buildAdvanced(modelViewProjection, lightVectorFromOrigin);
 if(hasSafeZone) {
  frustum.setMode(ShadowCullingFrustum::Mode::SafeZone);
  frustum.setBoxCuller(BoxCuller(distance));
  frustum.setDistanceCuller(BoxCuller(static_cast<double>(params.halfPlaneLength) * renderMultiplier));
  return frustum;
 }
 frustum.setMode(ShadowCullingFrustum::Mode::Advanced);
 if(distance < static_cast<double>(params.renderDistanceBlocks)) {
  frustum.setBoxCuller(BoxCuller(distance));
 }
 return frustum;
}
} // namespace net::minecraft::client::render
