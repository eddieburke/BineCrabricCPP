#pragma once
#include <algorithm>
#include <cstdint>
#include "net/minecraft/util/math/Types.hpp"
// Deterministic position-derived jitter, shared by anything keyed on a
// world position (blocks, entities, tile entities, particles, ...) that
// wants a stable per-coordinate variation instead of per-frame randomness.
namespace net::minecraft::util::math {
// Vanilla-style position seed: stable, cheap, and independent of tick/frame.
[[nodiscard]] inline std::int64_t coordinateSeed(int x, int y, int z) noexcept {
  return static_cast<std::int64_t>(x) * 3129871LL ^ static_cast<std::int64_t>(y) * 116129781LL ^
         static_cast<std::int64_t>(z);
}
[[nodiscard]] inline net::minecraft::JavaRandom coordinateRandom(int x, int y, int z) {
  return net::minecraft::JavaRandom(static_cast<std::uint64_t>(coordinateSeed(x, y, z)));
}
// 24-bit RGB hash derived from position; deliberately a different formula
// than coordinateSeed so color and scale/offset jitter don't correlate.
[[nodiscard]] inline int coordinateColor(int x, int y, int z) noexcept {
  const std::int64_t hash = static_cast<std::int64_t>(x) * x * 3187961LL + static_cast<std::int64_t>(x) * 987243LL +
                            static_cast<std::int64_t>(y) * y * 43297126LL + static_cast<std::int64_t>(y) * 987121LL +
                            static_cast<std::int64_t>(z) * z * 927469861LL + static_cast<std::int64_t>(z) * 1861LL;
  return static_cast<int>(hash & 0xFFFFFF);
}
// Draws a scale in [minScale, maxScale] from an already-seeded random, so
// callers can pull several jittered values (offset, scale, ...) from one
// coordinateRandom() without re-seeding.
[[nodiscard]] inline float coordinateScale(net::minecraft::JavaRandom& random, float minScale, float maxScale) {
  const float lo = std::min(minScale, maxScale);
  const float hi = std::max(minScale, maxScale);
  return lo + random.nextFloat() * (hi - lo);
}
// Draws a jittered offset in [-range/2, range/2] from an already-seeded
// random.
[[nodiscard]] inline float coordinateOffset(net::minecraft::JavaRandom& random, float range) {
  return random.nextFloat() * range - range * 0.5f;
}
} // namespace net::minecraft::util::math
