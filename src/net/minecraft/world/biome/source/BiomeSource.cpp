#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
namespace net::minecraft {
BiomeSource::ClimateSample BiomeSource::sampleClimate(int x, int z) const {
  std::vector<double> t(1);
  std::vector<double> d(1);
  std::vector<double> w(1);
  temperatureSampler_.sample(t, x, z, 1, 1, 0.025, 0.025, 0.25);
  downfallSampler_.sample(d, x, z, 1, 1, 0.05, 0.05, 0.3333333333333333);
  weirdnessSampler_.sample(w, x, z, 1, 1, 0.25, 0.25, 0.5882352941176471);
  const double weirdness = w[0] * 1.1 + 0.5;
  double temperature = (t[0] * 0.15 + 0.7) * 0.99 + weirdness * 0.01;
  double downfall = (d[0] * 0.15 + 0.5) * 0.998 + weirdness * 0.002;
  temperature = std::clamp(1.0 - (1.0 - temperature) * (1.0 - temperature), 0.0, 1.0);
  downfall = std::clamp(downfall, 0.0, 1.0);
  return {temperature, downfall};
}
Biome& BiomeSource::getBiome(int x, int z) {
  getBiomesInArea(queryScratch_, x, z, 1, 1);
  return queryScratch_.empty() ? Biome::getBiome(0.5, 0.5) : *queryScratch_.front();
}
std::vector<double>& BiomeSource::create(std::vector<double>& map, int x, int z, int width, int depth) {
  const std::size_t size = static_cast<std::size_t>(width * depth);
  if(map.size() < size) {
    map.resize(size);
  }
  temperatureSampler_.sample(map, x, z, width, depth, 0.025, 0.025, 0.25);
  weirdnessSampler_.sample(weirdnessMap_, x, z, width, depth, 0.25, 0.25, 0.5882352941176471);
  int index = 0;
  for(int ix = 0; ix < width; ++ix) {
    for(int iz = 0; iz < depth; ++iz) {
      const double weirdness = weirdnessMap_[static_cast<std::size_t>(index)] * 1.1 + 0.5;
      double temperature = (map[static_cast<std::size_t>(index)] * 0.15 + 0.7) * 0.99 + weirdness * 0.01;
      temperature = 1.0 - (1.0 - temperature) * (1.0 - temperature);
      if(temperature < 0.0) {
        temperature = 0.0;
      }
      if(temperature > 1.0) {
        temperature = 1.0;
      }
      map[static_cast<std::size_t>(index)] = temperature;
      ++index;
    }
  }
  return map;
}
std::vector<Biome*>& BiomeSource::getBiomesInArea(std::vector<Biome*>& biomes, int x, int z, int width, int depth) {
  const std::size_t size = static_cast<std::size_t>(width * depth);
  if(biomes.size() < size) {
    biomes.resize(size);
  }
  temperatureSampler_.sample(temperatureMap_, x, z, width, width, 0.025, 0.025, 0.25);
  downfallSampler_.sample(downfallMap_, x, z, width, width, 0.05, 0.05, 0.3333333333333333);
  weirdnessSampler_.sample(weirdnessMap_, x, z, width, width, 0.25, 0.25, 0.5882352941176471);
  int index = 0;
  for(int ix = 0; ix < width; ++ix) {
    for(int iz = 0; iz < depth; ++iz) {
      const double weirdness = weirdnessMap_[static_cast<std::size_t>(index)] * 1.1 + 0.5;
      double temperature =
          (temperatureMap_[static_cast<std::size_t>(index)] * 0.15 + 0.7) * 0.99 + weirdness * 0.01;
      double downfall = (downfallMap_[static_cast<std::size_t>(index)] * 0.15 + 0.5) * 0.998 + weirdness * 0.002;
      temperature = std::clamp(1.0 - (1.0 - temperature) * (1.0 - temperature), 0.0, 1.0);
      downfall = std::clamp(downfall, 0.0, 1.0);
      temperatureMap_[static_cast<std::size_t>(index)] = temperature;
      downfallMap_[static_cast<std::size_t>(index)] = downfall;
      biomes[static_cast<std::size_t>(index)] = &Biome::getBiome(temperature, downfall);
      ++index;
    }
  }
  return biomes;
}
} // namespace net::minecraft
