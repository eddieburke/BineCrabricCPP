#include "net/minecraft/client/render/culling/ShadowFrustum.hpp"
#include <cmath>
namespace net::minecraft::client::render {
namespace {
// Java BaseClippingPlanes: plane_i = normalize(transpose(P * MV) * corner_i), with the
// corners (-1,0,0,1), (1,0,0,1), (0,-1,0,1), (0,1,0,1), (0,0,-1,1), (0,0,1,1) — the
// left/right/bottom/top/far/near planes of the player's view volume. With column-major
// storage (m[column * 4 + row]) the transpose makes each plane one contiguous row.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/frustum/advanced/BaseClippingPlanes.java
std::array<float, 4> clippingPlane(const float m[16], float cornerX, float cornerY, float cornerZ) {
 std::array<float, 4> plane{};
 for(int row = 0; row < 4; ++row) {
  plane[static_cast<std::size_t>(row)] = m[row * 4 + 0] * cornerX + m[row * 4 + 1] * cornerY +
                                         m[row * 4 + 2] * cornerZ + m[row * 4 + 3];
 }
 // Java normalizes the Vector4f over all four components. Scaling a plane equation
 // uniformly leaves both the plane and every test below unchanged; kept for parity.
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
// Java NeighboringPlaneSet.forPlane: TABLE[planeIndex >>> 1], the four planes that share
// an edge with the given one.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/frustum/advanced/NeighboringPlaneSet.java
constexpr int kNeighboringPlanes[3][4] = {
    {2, 3, 4, 5}, // +/-X
    {0, 1, 4, 5}, // +/-Y
    {0, 1, 2, 3}, // +/-Z
};
} // namespace
void ShadowCullingFrustum::addPlane(const std::array<float, 4>& plane) noexcept {
 if(planeCount_ >= kMaxClippingPlanes) {
  return;
 }
 planes_[planeCount_] = plane;
 ++planeCount_;
}
// Java addEdgePlane: the plane through the intersection line of a back and a front
// plane whose normal is perpendicular to the light vector — the silhouette of the
// player's frustum as seen from the light, extruded infinitely toward it.
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
 // A point on the intersection line ("Line of intersection between two planes",
 // https://stackoverflow.com/a/32410473, as used by Java).
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
 planeCount_ = 0;
 lightVector_[0] = lightVectorFromOrigin[0];
 lightVector_[1] = lightVectorFromOrigin[1];
 lightVector_[2] = lightVectorFromOrigin[2];
 const std::array<std::array<float, 4>, 6> base = {
     clippingPlane(modelViewProjection, -1.0f, 0.0f, 0.0f), clippingPlane(modelViewProjection, 1.0f, 0.0f, 0.0f),
     clippingPlane(modelViewProjection, 0.0f, -1.0f, 0.0f), clippingPlane(modelViewProjection, 0.0f, 1.0f, 0.0f),
     clippingPlane(modelViewProjection, 0.0f, 0.0f, -1.0f), clippingPlane(modelViewProjection, 0.0f, 0.0f, 1.0f),
 };
 // Back planes: the ones facing the same general direction as the light vector. Those
 // bound the volume that can cast onto the player's frustum; the front planes are
 // replaced by the extruded edge planes below.
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
 if(hasBoxCuller_) {
  boxCuller_.setPosition(cameraX, cameraY, cameraZ);
 }
 if(hasDistanceCuller_) {
  distanceCuller_.setPosition(cameraX, cameraY, cameraZ);
 }
 x_ = cameraX;
 y_ = cameraY;
 z_ = cameraZ;
}
// Java checkCornerVisibility, ported from JOML's FrustumIntersection: a box is outside
// as soon as its most-positive corner along a plane normal falls behind that plane.
bool ShadowCullingFrustum::cornersVisible(float minX,
                                          float minY,
                                          float minZ,
                                          float maxX,
                                          float maxY,
                                          float maxZ) const noexcept {
 for(std::size_t i = 0; i < planeCount_; ++i) {
  const std::array<float, 4>& plane = planes_[i];
  const float boundX = plane[0] < 0.0f ? minX : maxX;
  const float boundY = plane[1] < 0.0f ? minY : maxY;
  const float boundZ = plane[2] < 0.0f ? minZ : maxZ;
  if(plane[0] * boundX + plane[1] * boundY + plane[2] * boundZ < -plane[3]) {
   return false;
  }
 }
 return true;
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
   // Java BoxCullingFrustum.isVisible.
   return !hasBoxCuller_ || !boxCuller_.isCulled(minX, minY, minZ, maxX, maxY, maxZ);
  case Mode::SafeZone:
   // Java SafeZoneCullingFrustum.isVisible: the distance box is a hard cut, and
   // anything inside the voxel box is kept regardless of the frustum.
   if(hasDistanceCuller_ && distanceCuller_.isCulled(minX, minY, minZ, maxX, maxY, maxZ)) {
    return false;
   }
   if(hasBoxCuller_ && !boxCuller_.isCulled(minX, minY, minZ, maxX, maxY, maxZ)) {
    return true;
   }
   break;
  case Mode::Advanced:
   // Java AdvancedShadowCullingFrustum.isVisible.
   if(hasBoxCuller_ && boxCuller_.isCulled(minX, minY, minZ, maxX, maxY, maxZ)) {
    return false;
   }
   break;
 }
 return cornersVisible(static_cast<float>(minX - x_), static_cast<float>(minY - y_),
                       static_cast<float>(minZ - z_), static_cast<float>(maxX - x_),
                       static_cast<float>(maxY - y_), static_cast<float>(maxZ - z_));
}
ShadowCullingFrustum createShadowFrustum(const ShadowFrustumParams& params,
                                         const float modelViewProjection[16],
                                         const float lightVectorFromOrigin[3]) {
 ShadowCullingFrustum frustum;
 const float renderDistanceBlocks = params.renderDistanceBlocks;
 if((params.cullState == ShadowCullState::Default && params.packHasVoxelization) ||
    params.cullState == ShadowCullState::Distance) {
  // Voxelizing packs read the shadow pass as a world volume, so no view-derived
  // culling is valid — only a distance box, and none at all when it would reach past
  // the render distance.
  const double distance = static_cast<double>(params.halfPlaneLength) * params.renderMultiplier;
  if(distance <= 0.0 || distance > static_cast<double>(renderDistanceBlocks)) {
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
  distance = static_cast<double>(params.userShadowDistanceChunks) * 16.0;
 }
 frustum.buildAdvanced(modelViewProjection, lightVectorFromOrigin);
 frustum.setMode(hasSafeZone ? ShadowCullingFrustum::Mode::SafeZone : ShadowCullingFrustum::Mode::Advanced);
 if(distance >= static_cast<double>(renderDistanceBlocks) && !hasSafeZone) {
  // The box would never cut anything the render distance has not already dropped.
  frustum.clearBoxCuller();
 } else {
  frustum.setBoxCuller(BoxCuller(distance));
 }
 if(hasSafeZone) {
  frustum.setDistanceCuller(BoxCuller(static_cast<double>(params.halfPlaneLength) * renderMultiplier));
 }
 return frustum;
}
} // namespace net::minecraft::client::render
