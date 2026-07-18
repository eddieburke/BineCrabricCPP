#pragma once

namespace net::minecraft::client::render::lod {
struct DistancePolicy {
  float baseDistance = 768.0f;
  int maxLevel = 4;

  [[nodiscard]] int level(float distance, int detail) const noexcept;
};
}
