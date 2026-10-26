#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
namespace net::minecraft::world::gen {
enum class TerrainStyle {
 Beta = 0,
 Alpha,
 Modern,
};
[[nodiscard]] std::string_view terrainStyleName(TerrainStyle style);
[[nodiscard]] TerrainStyle parseTerrainStyle(std::string_view name);
[[nodiscard]] std::string terrainStyleOptionKey(int dimensionId);
[[nodiscard]] TerrainStyle terrainStyleFromOptions(
    const std::unordered_map<std::string, std::string>& modOptions, int dimensionId);
[[nodiscard]] inline double terrainClimateFactor(TerrainStyle style, double temperature, double downfall) {
 if(style == TerrainStyle::Alpha) {
  return 1.0;
 }
 double factor = 1.0 - downfall * temperature;
 factor *= factor;
 factor *= factor;
 return 1.0 - factor;
}
} // namespace net::minecraft::world::gen
