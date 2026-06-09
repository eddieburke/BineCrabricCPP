#pragma once

#include "net/minecraft/world/biome/BasicBiome.hpp"
#include "net/minecraft/world/biome/BiomeDefinition.hpp"
#include "net/minecraft/world/biome/ForestBiome.hpp"
#include "net/minecraft/world/biome/HellBiome.hpp"
#include "net/minecraft/world/biome/RainforestBiome.hpp"
#include "net/minecraft/world/biome/SkyBiome.hpp"
#include "net/minecraft/world/biome/SwamplandBiome.hpp"
#include "net/minecraft/world/biome/TaigaBiome.hpp"

namespace net::minecraft {

// Static biome registry matching net.minecraft.world.biome.Biome static fields.
class Biomes {
public:
    static void init();

    static BiomeDefinition& rainforest();
    static BiomeDefinition& swampland();
    static BiomeDefinition& seasonalForest();
    static BiomeDefinition& forest();
    static BiomeDefinition& savanna();
    static BiomeDefinition& shrubland();
    static BiomeDefinition& taiga();
    static BiomeDefinition& desert();
    static BiomeDefinition& plains();
    static BiomeDefinition& iceDesert();
    static BiomeDefinition& tundra();
    static BiomeDefinition& hell();
    static BiomeDefinition& sky();

    [[nodiscard]] static BiomeDefinition& byId(BiomeId id);
    [[nodiscard]] static BiomeDefinition& byInfo(const BiomeInfo& info) { return byId(info.id); }

    // Faithful to Biome.getBiome: quantize climate to a 64x64 lookup table.
    [[nodiscard]] static BiomeInfo getBiome(double temperature, double downfall);
};

} // namespace net::minecraft
