#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
namespace net::minecraft::client::render {
// A set of clipping planes and the AABB test against them. ONE implementation.
//
// The view frustum (6 planes, extracted from a clip matrix) and the advanced
// shadow frustum (up to 13, built by extruding the view frustum along the light)
// differ only in how their planes are produced and how many there are. The test
// itself — for each plane, take the box corner furthest along the plane normal
// and check which side it falls on — was written twice, once branchless with an
// epsilon and once with ternaries and none, which is how two culling paths end
// up disagreeing about the same box.
template <std::size_t MaxPlanes>
class PlaneSet {
 public:
 void clear() noexcept {
  count_ = 0;
 }
 // Silently ignores planes past capacity, matching Iris' AdvancedShadowCulling-
 // Frustum: the plane budget is a fixed geometric maximum, so overflow means a
 // build bug, not a runtime condition to handle.
 void add(const std::array<float, 4>& plane) noexcept {
  if(count_ >= MaxPlanes) {
   return;
  }
  planes_[count_] = plane;
  ++count_;
 }
 [[nodiscard]] std::size_t count() const noexcept {
  return count_;
 }
 [[nodiscard]] const std::array<float, 4>& plane(std::size_t index) const noexcept {
  return planes_[index];
 }
 [[nodiscard]] std::array<float, 4>& plane(std::size_t index) noexcept {
  return planes_[index];
 }
 void setCount(std::size_t count) noexcept {
  count_ = count > MaxPlanes ? MaxPlanes : count;
 }
 // True when the box is on the inside (or straddling) every plane. `epsilon`
 // widens the frustum slightly: the view frustum keeps boxes that only graze a
 // plane, so a section touching the edge does not pop.
 [[nodiscard]] bool intersectsAabb(float minX,
                                   float minY,
                                   float minZ,
                                   float maxX,
                                   float maxY,
                                   float maxZ,
                                   float epsilon = 0.0f) const noexcept {
  for(std::size_t i = 0; i < count_; ++i) {
   const std::array<float, 4>& p = planes_[i];
   const float px = pickBound(p[0], maxX, minX);
   const float py = pickBound(p[1], maxY, minY);
   const float pz = pickBound(p[2], maxZ, minZ);
   if(((p[0] * px + p[1] * py) + p[2] * pz) + p[3] <= -epsilon) {
    return false;
   }
  }
  return true;
 }

 private:
 // Branchless select of the corner furthest along the normal: hi when the
 // component is positive, lo when negative. Sign-extend the float's sign bit to
 // a full mask rather than branching per plane per box.
 static float pickBound(float sign, float hi, float lo) noexcept {
  const std::int32_t sel = static_cast<std::int32_t>(std::bit_cast<std::uint32_t>(sign)) >> 31;
  return std::bit_cast<float>((std::bit_cast<std::uint32_t>(hi) & ~static_cast<std::uint32_t>(sel)) |
                              (std::bit_cast<std::uint32_t>(lo) & static_cast<std::uint32_t>(sel)));
 }
 std::array<std::array<float, 4>, MaxPlanes> planes_{};
 std::size_t count_ = 0;
};
} // namespace net::minecraft::client::render
