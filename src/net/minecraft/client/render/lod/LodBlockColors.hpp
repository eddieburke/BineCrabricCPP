#pragma once
#include <array>
#include <cstdint>
namespace net::minecraft::client::render::lod {
class LodBlockColors {
public:
  [[nodiscard]] static const std::array<std::uint32_t, 256>& table();
  [[nodiscard]] static const std::array<std::uint32_t, 256>& sideTable();
  static void invalidate();
};
} // namespace net::minecraft::client::render::lod
