#include "net/minecraft/world/biome/BiomeNames.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include <cctype>
namespace net::minecraft {
namespace {
constexpr const char* kSpawnPickerOrder[] = {
    "plains",
    "forest",
    "desert",
    "taiga",
    "swampland",
    "rainforest",
    "savanna",
    "shrubland",
    "seasonal_forest",
    "ice_desert",
    "tundra",
    "hell",
    "sky",
};
std::string normalize(std::string_view name) {
  std::string out;
  out.reserve(name.size());
  for(char ch : name) {
    if(ch == ' ') {
      out.push_back('_');
    } else {
      out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
  }
  return out;
}
} // namespace
std::string biomeWireName(int biomeId) {
  if(biomeId < 0 || biomeId >= kBiomeCount) {
    return "unknown";
  }
  return std::string(Biome::byId(static_cast<BiomeId>(biomeId)).wireName());
}
std::vector<std::string> allBiomeWireNames() {
  std::vector<std::string> names;
  names.reserve(kBiomeCount);
  for(int i = 0; i < kBiomeCount; ++i) {
    names.emplace_back(Biome::byId(static_cast<BiomeId>(i)).wireName());
  }
  return names;
}
std::vector<std::string> spawnPickerBiomeWireNames() {
  return {std::begin(kSpawnPickerOrder), std::end(kSpawnPickerOrder)};
}
int biomeIdFromWireName(std::string_view name) {
  const std::string normalized = normalize(name);
  for(int i = 0; i < kBiomeCount; ++i) {
    const Biome& biome = Biome::byId(static_cast<BiomeId>(i));
    if(normalized == biome.wireName() || normalized == normalize(biome.name)) {
      return i;
    }
  }
  return 0;
}
} // namespace net::minecraft
