#pragma once
#include <array>
namespace net::minecraft::util::math {
// Faithful port of net.minecraft.util.math.Facings (beta 1.7.3).
struct Facings {
  static constexpr std::array<int, 4> TO_DIR = {3, 4, 2, 5};
  static constexpr std::array<int, 4> OPPOSITE = {2, 3, 0, 1};
  static constexpr std::array<std::array<int, 6>, 4> BED_FACINGS = {{
      {{1, 0, 3, 2, 5, 4}},
      {{1, 0, 5, 4, 2, 3}},
      {{1, 0, 2, 3, 4, 5}},
      {{1, 0, 4, 5, 3, 2}},
  }};
};
} // namespace net::minecraft::util::math
