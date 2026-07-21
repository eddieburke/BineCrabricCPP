#pragma once
#include <algorithm>
#include <array>
#include <span>
namespace net::minecraft::client::color::world {
class WaterColors {
 public:
 static void setColorMap(std::span<const int> colorMap) {
  auto& map = mutableColorMap();
  const std::size_t count = std::min(map.size(), colorMap.size());
  for(std::size_t i = 0; i < count; ++i) {
   map[i] = colorMap[i];
  }
 }

 private:
 [[nodiscard]] static std::array<int, 65536>& mutableColorMap() {
  static std::array<int, 65536> colorMap{};
  return colorMap;
 }
};
} // namespace net::minecraft::client::color::world
