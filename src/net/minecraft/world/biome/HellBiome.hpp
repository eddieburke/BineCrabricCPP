#pragma once

#include "net/minecraft/world/biome/BiomeDefinition.hpp"

namespace net::minecraft {

class HellBiome : public BiomeDefinition {
public:
    HellBiome()
    {
        spawnableMonsters_.clear();
        spawnablePassive_.clear();
        spawnableWaterCreatures_.clear();
        spawnableMonsters_.push_back({"Ghast", 10});
        spawnableMonsters_.push_back({"PigZombie", 10});
    }
};

} // namespace net::minecraft
