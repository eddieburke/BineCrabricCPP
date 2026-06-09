#pragma once

#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/EntitySpawnGroup.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace net::minecraft {

class Feature;

// Faithful port of net.minecraft.world.biome.Biome (beta 1.7.3).
class BiomeDefinition {
public:
    BiomeDefinition();

    BiomeDefinition& setName(std::string value) { name = std::move(value); return *this; }
    BiomeDefinition& setGrassColor(int value) { grassColor = value; return *this; }
    BiomeDefinition& setFoliageColor(int value) { foliageColor = value; return *this; }
    BiomeDefinition& enableSnow() { hasSnow = true; return *this; }
    BiomeDefinition& disableRain() { hasRain = false; return *this; }

    [[nodiscard]] virtual std::unique_ptr<Feature> getRandomTreeFeature(JavaRandom& random);
    [[nodiscard]] virtual int getSkyColor(float brightness) const;

    [[nodiscard]] const std::vector<EntitySpawnGroup>& getSpawnableEntities(int group) const;
    [[nodiscard]] bool canSnow() const noexcept { return hasSnow; }
    [[nodiscard]] bool canRain() const noexcept { return hasRain && !hasSnow; }

    BiomeId id = BiomeId::Plains;
    std::string name = "Plains";
    int grassColor = 0;
    std::uint8_t topBlockId = 2;
    std::uint8_t soilBlockId = 3;
    int foliageColor = 5169201;

protected:
    std::vector<EntitySpawnGroup> spawnableMonsters_;
    std::vector<EntitySpawnGroup> spawnablePassive_;
    std::vector<EntitySpawnGroup> spawnableWaterCreatures_;

private:
    bool hasSnow = false;
    bool hasRain = true;
};

} // namespace net::minecraft
