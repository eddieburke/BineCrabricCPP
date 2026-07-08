#pragma once
#include <algorithm>
#include <memory>
#include <vector>

#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

namespace net::minecraft {
class FixedBiomeSource : public BiomeSource {
   public:
    FixedBiomeSource(Biome& biome, double temperature, double downfall)
        : BiomeSource(0ULL), biome_(biome), temperature_(temperature), downfall_(downfall) {
    }

    [[nodiscard]] std::unique_ptr<BiomeSource> clone() const override {
        return std::make_unique<FixedBiomeSource>(biome_, temperature_, downfall_);
    }

    [[nodiscard]] Biome& getBiome(int x, int z) override {
        (void) x;
        (void) z;
        return biome_;
    }

    [[nodiscard]] double getTemperature(int x, int z) override {
        (void) x;
        (void) z;
        return temperature_;
    }

    std::vector<Biome*>& getBiomesInArea(std::vector<Biome*>& biomes, int x, int z, int width, int depth) override {
        (void) x;
        (void) z;
        const std::size_t size = static_cast<std::size_t>(width * depth);
        if (biomes.size() < size) {
            biomes.resize(size);
        }
        if (temperatureMap_.size() < size) {
            temperatureMap_.resize(size);
            downfallMap_.resize(size);
        }
        std::fill(biomes.begin(), biomes.begin() + static_cast<std::ptrdiff_t>(size), &biome_);
        std::fill(temperatureMap_.begin(), temperatureMap_.begin() + static_cast<std::ptrdiff_t>(size), temperature_);
        std::fill(downfallMap_.begin(), downfallMap_.begin() + static_cast<std::ptrdiff_t>(size), downfall_);
        return biomes;
    }

    std::vector<double>& create(std::vector<double>& map, int x, int z, int width, int depth) override {
        (void) x;
        (void) z;
        const std::size_t size = static_cast<std::size_t>(width * depth);
        if (map.size() < size) {
            map.resize(size);
        }
        std::fill(map.begin(), map.begin() + static_cast<std::ptrdiff_t>(size), temperature_);
        return map;
    }

   private:
    Biome& biome_;
    double temperature_ = 0.0;
    double downfall_ = 0.0;
};
}  // namespace net::minecraft
