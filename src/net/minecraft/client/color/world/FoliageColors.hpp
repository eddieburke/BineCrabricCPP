#pragma once
#include <algorithm>
#include <array>
#include <functional>
#include <span>
#include <utility>
namespace net::minecraft::client::color::world {
class FoliageColors {
public:
  static void setColorMap(std::span<const int> colorMap) {
    auto& map = mutableColorMap();
    const std::size_t count = std::min(map.size(), colorMap.size());
    for(std::size_t i = 0; i < count; ++i) {
      map[i] = colorMap[i];
    }
  }
  // See GrassColors::setColorOverride. Leaf tint remap registered by world-profile themes.
  static void setColorOverride(std::function<int(int)> override) {
    colorOverride() = std::move(override);
  }
  [[nodiscard]] static int getColor(double temperature, double humidity) {
    const int x = static_cast<int>((1.0 - temperature) * 255.0);
    humidity *= temperature;
    const int y = static_cast<int>((1.0 - humidity) * 255.0);
    const int base = mutableColorMap()[static_cast<std::size_t>((y << 8) | x)];
    return applyOverride(base);
  }
  [[nodiscard]] static int getSpruceColor() {
    return applyOverride(0x619961);
  }
  [[nodiscard]] static int getBirchColor() {
    return applyOverride(8431445);
  }
  [[nodiscard]] static int getDefaultColor() {
    return applyOverride(4764952);
  }

private:
  [[nodiscard]] static int applyOverride(int base) {
    const auto& override = colorOverride();
    return override ? override(base) : base;
  }
  [[nodiscard]] static std::array<int, 65536>& mutableColorMap() {
    static std::array<int, 65536> colorMap{};
    return colorMap;
  }
  [[nodiscard]] static std::function<int(int)>& colorOverride() {
    static std::function<int(int)> override;
    return override;
  }
};
} // namespace net::minecraft::client::color::world
