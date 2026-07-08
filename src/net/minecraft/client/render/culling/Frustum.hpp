#pragma once
#include "net/minecraft/util/math/Types.hpp"
#include <array>
namespace net::minecraft::client::render {
class Frustum {
public:
  static Frustum& getInstance();
  void compute();
  [[nodiscard]] bool intersects(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) const {
    constexpr double kPlaneEpsilon = 1.0e-5;
    for(int i = 0; i < 6; ++i) {
      const double a = frustum[static_cast<std::size_t>(i)][0];
      const double b = frustum[static_cast<std::size_t>(i)][1];
      const double c = frustum[static_cast<std::size_t>(i)][2];
      const double d = frustum[static_cast<std::size_t>(i)][3];
      if(a * minX + b * minY + c * minZ + d > -kPlaneEpsilon) {
        continue;
      }
      if(a * maxX + b * minY + c * minZ + d > -kPlaneEpsilon) {
        continue;
      }
      if(a * minX + b * maxY + c * minZ + d > -kPlaneEpsilon) {
        continue;
      }
      if(a * maxX + b * maxY + c * minZ + d > -kPlaneEpsilon) {
        continue;
      }
      if(a * minX + b * minY + c * maxZ + d > -kPlaneEpsilon) {
        continue;
      }
      if(a * maxX + b * minY + c * maxZ + d > -kPlaneEpsilon) {
        continue;
      }
      if(a * minX + b * maxY + c * maxZ + d > -kPlaneEpsilon) {
        continue;
      }
      if(a * maxX + b * maxY + c * maxZ + d > -kPlaneEpsilon) {
        continue;
      }
      return false;
    }
    return true;
  }
  std::array<std::array<float, 4>, 6> frustum{};
  std::array<float, 16> projectionMatrix{};
  std::array<float, 16> modelMatrix{};
  std::array<float, 16> clipMatrix{};

private:
  static void normalize(float plane[4]);
};
class FrustumCuller {
public:
  void prepare(double x, double y, double z) noexcept {
    offsetX_ = x;
    offsetY_ = y;
    offsetZ_ = z;
    frustum_ = &Frustum::getInstance();
  }
  [[nodiscard]] bool isVisible(const net::minecraft::Box& box) const noexcept {
    if(frustum_ == nullptr) {
      return true;
    }
    return frustum_->intersects(box.minX - offsetX_, box.minY - offsetY_, box.minZ - offsetZ_, box.maxX - offsetX_,
                                box.maxY - offsetY_, box.maxZ - offsetZ_);
  }

private:
  const Frustum* frustum_ = nullptr;
  double offsetX_ = 0.0;
  double offsetY_ = 0.0;
  double offsetZ_ = 0.0;
};
} // namespace net::minecraft::client::render
