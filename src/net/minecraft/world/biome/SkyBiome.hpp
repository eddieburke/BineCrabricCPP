#pragma once

#include "net/minecraft/world/biome/BiomeDefinition.hpp"

namespace net::minecraft {

class SkyBiome : public BiomeDefinition {
public:
    SkyBiome()
    {
        spawnableMonsters_.clear();
        spawnablePassive_.clear();
        spawnableWaterCreatures_.clear();
        spawnablePassive_.push_back({"Chicken", 10});
    }

    [[nodiscard]] int getSkyColor(float brightness) const override
    {
        (void)brightness;
        return 0xC0C0FF;
    }
};

} // namespace net::minecraft
