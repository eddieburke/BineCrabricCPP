#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/util/math/noise/OctaveSimplexNoiseSampler.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
namespace net::minecraft {
class BiomeSource {
public:
  explicit BiomeSource(std::uint64_t seed)
      : seed_(seed),
        temperatureRandom_(seed * 9871ULL),
        downfallRandom_(seed * 39811ULL),
        weirdnessRandom_(seed * 543321ULL),
        temperatureSampler_(temperatureRandom_, 4),
        downfallSampler_(downfallRandom_, 4),
        weirdnessSampler_(weirdnessRandom_, 2) {
  }
  void setSeed(std::uint64_t seed) {
    seed_ = seed;
    temperatureRandom_.setSeed(seed * 9871ULL);
    downfallRandom_.setSeed(seed * 39811ULL);
    weirdnessRandom_.setSeed(seed * 543321ULL);
    temperatureSampler_ = OctaveSimplexNoiseSampler(temperatureRandom_, 4);
    downfallSampler_ = OctaveSimplexNoiseSampler(downfallRandom_, 4);
    weirdnessSampler_ = OctaveSimplexNoiseSampler(weirdnessRandom_, 2);
    temperatureMap_.clear();
    downfallMap_.clear();
    weirdnessMap_.clear();
    queryScratch_.clear();
  }
  virtual ~BiomeSource() = default;
  // Fresh source with identical sampling state. Mesh-snapshot jobs clone the
  // dimension's source so worker threads never touch its mutable scratch maps.
  [[nodiscard]] virtual std::unique_ptr<BiomeSource> clone() const {
    return std::make_unique<BiomeSource>(seed_);
  }
  [[nodiscard]] const std::vector<double>& temperatureMap() const noexcept {
    return temperatureMap_;
  }
  [[nodiscard]] const std::vector<double>& downfallMap() const noexcept {
    return downfallMap_;
  }
  // Single-point climate query with no member mutation: const and thread-safe,
  // so World no longer needs a mutable scratch vector per temperature/downfall
  // lookup. Byte-identical to getBiomesInArea's per-cell result.
  struct ClimateSample {
    double temperature = 0.5;
    double downfall = 0.5;
  };
  [[nodiscard]] ClimateSample sampleClimate(int x, int z) const;
  [[nodiscard]] virtual Biome& getBiome(int x, int z);
  [[nodiscard]] virtual double getTemperature(int x, int z) {
    temperatureSampler_.sample(temperatureMap_, x, z, 1, 1, 0.025, 0.025, 0.5);
    return temperatureMap_.empty() ? 0.5 : temperatureMap_.front();
  }
  // Faithful to Java BiomeSource.create: fills a temperature map for an area,
  // used by the snow-layer decoration pass.
  virtual std::vector<double>& create(std::vector<double>& map, int x, int z, int width, int depth);
  virtual std::vector<Biome*>& getBiomesInArea(std::vector<Biome*>& biomes, int x, int z, int width, int depth);

protected:
  std::vector<double> temperatureMap_;
  std::vector<double> downfallMap_;

private:
  std::uint64_t seed_ = 0;
  JavaRandom temperatureRandom_;
  JavaRandom downfallRandom_;
  JavaRandom weirdnessRandom_;
  OctaveSimplexNoiseSampler temperatureSampler_;
  OctaveSimplexNoiseSampler downfallSampler_;
  OctaveSimplexNoiseSampler weirdnessSampler_;
  std::vector<double> weirdnessMap_;
  mutable std::vector<Biome*> queryScratch_;
};
} // namespace net::minecraft
