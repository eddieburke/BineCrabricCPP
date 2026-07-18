#include "net/minecraft/client/render/lod/LodPolicy.hpp"
#include <algorithm>

namespace net::minecraft::client::render::lod {
int DistancePolicy::level(float distance, int detail) const noexcept {
  const float scale = detail < 0 ? 0.5f : detail > 0 ? 2.0f : 1.0f;
  const float firstThreshold = std::max(1.0f, baseDistance * scale);
  int selected = 0;
  float threshold = firstThreshold;
  while(selected < std::max(0, maxLevel) && distance >= threshold) {
    ++selected;
    threshold *= 2.0f;
  }
  return selected;
}
}
