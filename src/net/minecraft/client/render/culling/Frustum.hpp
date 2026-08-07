#pragma once
#include "net/minecraft/client/render/culling/PlaneSet.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render {
class Frustum {
 public:
 void compute(const net::minecraft::util::math::Matrix4f& projection,
              const net::minecraft::util::math::Matrix4f& modelView);
 void prepare(double x, double y, double z) noexcept {
  x_ = x;
  y_ = y;
  z_ = z;
 }
 [[nodiscard]] bool isVisible(const net::minecraft::Box& box) const noexcept {
  return isVisible(box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
 }
 [[nodiscard]] bool isVisible(double minX,
                              double minY,
                              double minZ,
                              double maxX,
                              double maxY,
                              double maxZ) const noexcept {
  constexpr float kPlaneEpsilon = 1.0e-5f;
  return planes_.intersectsAabb(static_cast<float>(minX - x_), static_cast<float>(minY - y_),
                                static_cast<float>(minZ - z_), static_cast<float>(maxX - x_),
                                static_cast<float>(maxY - y_), static_cast<float>(maxZ - z_),
                                kPlaneEpsilon);
 }

 private:
 static void normalize(float plane[4]);
 PlaneSet<6> planes_{};
 double x_ = 0.0;
 double y_ = 0.0;
 double z_ = 0.0;
};
} // namespace net::minecraft::client::render
