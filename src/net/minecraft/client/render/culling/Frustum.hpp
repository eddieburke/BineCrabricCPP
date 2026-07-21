#pragma once
#include <array>
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render {
class Frustum {
 public:
 static Frustum& getInstance();
 void compute();
 [[nodiscard]] bool intersects(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) const {
  constexpr float kPlaneEpsilon = 1.0e-5f;
  for(int i = 0; i < 6; ++i) {
   const float a = frustum[static_cast<std::size_t>(i)][0];
   const float b = frustum[static_cast<std::size_t>(i)][1];
   const float c = frustum[static_cast<std::size_t>(i)][2];
   const float d = frustum[static_cast<std::size_t>(i)][3];
   const float px = (a >= 0.0f) ? maxX : minX;
   const float py = (b >= 0.0f) ? maxY : minY;
   const float pz = (c >= 0.0f) ? maxZ : minZ;
   if(a * px + b * py + c * pz + d <= -kPlaneEpsilon) {
    return false;
   }
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
   return frustum_->intersects(static_cast<float>(box.minX - offsetX_),
                               static_cast<float>(box.minY - offsetY_),
                               static_cast<float>(box.minZ - offsetZ_),
                               static_cast<float>(box.maxX - offsetX_),
                               static_cast<float>(box.maxY - offsetY_),
                               static_cast<float>(box.maxZ - offsetZ_));
  }

 private:
 const Frustum* frustum_ = nullptr;
 double offsetX_ = 0.0;
 double offsetY_ = 0.0;
 double offsetZ_ = 0.0;
};
} // namespace net::minecraft::client::render
