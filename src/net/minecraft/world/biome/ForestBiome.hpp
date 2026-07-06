#pragma once
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/gen/feature/BirchTreeFeature.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/LargeOakTreeFeature.hpp"
#include "net/minecraft/world/gen/feature/OakTreeFeature.hpp"
namespace net::minecraft {
class ForestBiome : public Biome {
public:
  ForestBiome() {
    spawnablePassive_.push_back({"Wolf", 2});
  }
  [[nodiscard]] std::unique_ptr<Feature> getRandomTreeFeature(JavaRandom& random) const override {
    if(random.nextInt(5) == 0) {
      return std::make_unique<BirchTreeFeature>();
    }
    if(random.nextInt(3) == 0) {
      return std::make_unique<LargeOakTreeFeature>();
    }
    return std::make_unique<OakTreeFeature>();
  }
};
} // namespace net::minecraft
