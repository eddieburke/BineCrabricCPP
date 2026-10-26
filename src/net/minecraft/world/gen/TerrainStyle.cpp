#include "net/minecraft/world/gen/TerrainStyle.hpp"
namespace net::minecraft::world::gen {
std::string_view terrainStyleName(TerrainStyle style) {
 switch(style) {
 case TerrainStyle::Alpha:
  return "alpha";
 case TerrainStyle::Modern:
  return "modern";
 case TerrainStyle::Beta:
  break;
 }
 return "beta";
}
TerrainStyle parseTerrainStyle(std::string_view name) {
 if(name == "alpha") {
  return TerrainStyle::Alpha;
 }
 if(name == "modern") {
  return TerrainStyle::Modern;
 }
 return TerrainStyle::Beta;
}
std::string terrainStyleOptionKey(int dimensionId) {
 return "terrain_style:" + std::to_string(dimensionId);
}
TerrainStyle terrainStyleFromOptions(const std::unordered_map<std::string, std::string>& modOptions, int dimensionId) {
 const auto found = modOptions.find(terrainStyleOptionKey(dimensionId));
 if(found == modOptions.end()) {
  return TerrainStyle::Beta;
 }
 return parseTerrainStyle(found->second);
}
} // namespace net::minecraft::world::gen
