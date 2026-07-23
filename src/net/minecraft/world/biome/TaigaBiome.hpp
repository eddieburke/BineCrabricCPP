#pragma once
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/PineTreeFeature.hpp"
#include "net/minecraft/world/gen/feature/SpruceTreeFeature.hpp"
namespace net::minecraft {
class TaigaBiome : public Biome {
 public:
 TaigaBiome() {
  spawnablePassive_.push_back({"Wolf", 2});
 }
 [[nodiscard]] std::unique_ptr<Feature> getRandomTreeFeature(JavaRandom& random) const override {
  if(random.nextInt(3) == 0) {
   return std::make_unique<PineTreeFeature>();
  }
  return std::make_unique<SpruceTreeFeature>();
 }
};
} // namespace net::minecraft
