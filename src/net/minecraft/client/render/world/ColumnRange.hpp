#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace net::minecraft::client::render::world {
template <class Visit>
void forEachEnteringColumn(int cameraX, int cameraZ, int radius,
                           int oldX, int oldZ, int oldRadius, Visit&& visit) {
 for(int dz = -radius; dz <= radius; ++dz) {
  const int width = static_cast<int>(std::sqrt(static_cast<double>(radius) * radius - static_cast<double>(dz) * dz));
  const int z = cameraZ + dz;
  const int left = cameraX - width;
  const int right = cameraX + width;
  const auto oldDz = static_cast<std::int64_t>(z) - oldZ;
  if(oldRadius < 0 || std::abs(oldDz) > oldRadius) {
   for(int x = left; x <= right; ++x) visit(x, z);
   continue;
  }
  const int oldWidth = static_cast<int>(std::sqrt(static_cast<double>(oldRadius) * oldRadius - static_cast<double>(oldDz) * oldDz));
  const auto oldLeft = static_cast<std::int64_t>(oldX) - oldWidth;
  const auto oldRight = static_cast<std::int64_t>(oldX) + oldWidth;
  for(int x = left; x <= right && x < oldLeft; ++x) visit(x, z);
  for(auto x = std::max(static_cast<std::int64_t>(left), oldRight + 1); x <= right; ++x) visit(static_cast<int>(x), z);
 }
}
}
