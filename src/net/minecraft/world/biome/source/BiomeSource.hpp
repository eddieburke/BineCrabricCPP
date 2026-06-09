#pragma once

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/util/math/noise/OctaveSimplexNoiseSampler.hpp"
#include "net/minecraft/world/biome/Biome.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace net::minecraft {

class BiomeSource {
public:
    explicit BiomeSource(std::uint64_t seed)
        : temperatureRandom_(seed * 9871ULL),
          downfallRandom_(seed * 39811ULL),
          weirdnessRandom_(seed * 543321ULL),
          temperatureSampler_(temperatureRandom_, 4),
          downfallSampler_(downfallRandom_, 4),
          weirdnessSampler_(weirdnessRandom_, 2)
    {
    }

    void setSeed(std::uint64_t seed)
    {
        temperatureRandom_.setSeed(seed * 9871ULL);
        downfallRandom_.setSeed(seed * 39811ULL);
        weirdnessRandom_.setSeed(seed * 543321ULL);
    }

    virtual ~BiomeSource() = default;

    [[nodiscard]] const std::vector<double>& temperatureMap() const noexcept { return temperatureMap_; }
    [[nodiscard]] const std::vector<double>& downfallMap() const noexcept { return downfallMap_; }

    [[nodiscard]] virtual BiomeInfo getBiome(int x, int z);

    [[nodiscard]] virtual double getTemperature(int x, int z)
    {
        temperatureSampler_.sample(temperatureMap_, x, z, 1, 1, 0.025, 0.025, 0.5);
        return temperatureMap_.empty() ? 0.5 : temperatureMap_.front();
    }

    // Faithful to Java BiomeSource.create: fills a temperature map for an area,
    // used by the snow-layer decoration pass.
    virtual std::vector<double>& create(std::vector<double>& map, int x, int z, int width, int depth);

    virtual std::vector<BiomeInfo>& getBiomesInArea(std::vector<BiomeInfo>& biomes, int x, int z, int width, int depth);

protected:
    std::vector<double> temperatureMap_;
    std::vector<double> downfallMap_;

private:
    JavaRandom temperatureRandom_;
    JavaRandom downfallRandom_;
    JavaRandom weirdnessRandom_;
    OctaveSimplexNoiseSampler temperatureSampler_;
    OctaveSimplexNoiseSampler downfallSampler_;
    OctaveSimplexNoiseSampler weirdnessSampler_;
    std::vector<double> weirdnessMap_;
    mutable std::vector<BiomeInfo> queryScratch_;
};

} // namespace net::minecraft
