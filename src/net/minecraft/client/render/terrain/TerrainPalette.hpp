#pragma once
#include <array>
#include <cstdint>
namespace net::minecraft::client::render::terrain {
class TerrainPalette {
public:
  [[nodiscard]] static const std::array<std::uint32_t, 256>& table();
  [[nodiscard]] static const std::array<std::uint32_t, 256>& sideTable();
  [[nodiscard]] static std::uint64_t generation() noexcept;
  static void invalidate();
};
} // namespace net::minecraft::client::render::terrain
