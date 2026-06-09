#include "net/minecraft/world/biome/Biomes.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/biome/Biome.hpp"

#include <array>

namespace net::minecraft {

namespace {
std::array<BiomeInfo, 4096> gBiomeLookup {};
RainforestBiome gRainforest;
SwamplandBiome gSwampland;
BiomeDefinition gSeasonalForest;
ForestBiome gForest;
BasicBiome gSavanna;
BiomeDefinition gShrubland;
TaigaBiome gTaiga;
BasicBiome gDesert;
BasicBiome gPlains;
BasicBiome gIceDesert;
BiomeDefinition gTundra;
HellBiome gHell;
SkyBiome gSky;
bool gInitialized = false;
}

void Biomes::init()
{
    if (gInitialized) {
        return;
    }
    gInitialized = true;

    gRainforest.id = BiomeId::Rainforest;
    gRainforest.setGrassColor(588342).setName("Rainforest").setFoliageColor(2094168);

    gSwampland.id = BiomeId::Swampland;
    gSwampland.setGrassColor(522674).setName("Swampland").setFoliageColor(9154376);

    gSeasonalForest.id = BiomeId::SeasonalForest;
    gSeasonalForest.setGrassColor(10215459).setName("Seasonal Forest");

    gForest.id = BiomeId::Forest;
    gForest.setGrassColor(353825).setName("Forest").setFoliageColor(5159473);

    gSavanna.id = BiomeId::Savanna;
    gSavanna.setGrassColor(14278691).setName("Savanna");

    gShrubland.id = BiomeId::Shrubland;
    gShrubland.setGrassColor(10595616).setName("Shrubland");

    gTaiga.id = BiomeId::Taiga;
    gTaiga.setGrassColor(3060051).setName("Taiga").enableSnow().setFoliageColor(8107825);

    gDesert.id = BiomeId::Desert;
    gDesert.setGrassColor(16421912).setName("Desert").disableRain();

    gPlains.id = BiomeId::Plains;
    gPlains.setGrassColor(16767248).setName("Plains");

    gIceDesert.id = BiomeId::IceDesert;
    gIceDesert.setGrassColor(16772499).setName("Ice Desert").enableSnow().disableRain().setFoliageColor(12899129);

    gTundra.id = BiomeId::Tundra;
    gTundra.setGrassColor(5762041).setName("Tundra").enableSnow().setFoliageColor(12899129);

    gHell.id = BiomeId::Hell;
    gHell.setGrassColor(0xFF0000).setName("Hell").disableRain();

    gSky.id = BiomeId::Sky;
    gSky.setGrassColor(0x8080FF).setName("Sky").disableRain();

    if (Block::SAND != nullptr) {
        const auto sandId = static_cast<std::uint8_t>(Block::SAND->id);
        gDesert.topBlockId = sandId;
        gDesert.soilBlockId = sandId;
        gIceDesert.topBlockId = sandId;
        gIceDesert.soilBlockId = sandId;
    }

    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 64; ++j) {
            const BiomeInfo located = locateBiome(static_cast<double>(i) / 63.0, static_cast<double>(j) / 63.0);
            const BiomeDefinition& def = byId(located.id);
            gBiomeLookup[static_cast<std::size_t>(i + j * 64)] = {def.id, def.name, def.topBlockId, def.soilBlockId};
        }
    }
}

BiomeInfo Biomes::getBiome(double temperature, double downfall)
{
    init();
    const int temperatureIndex = static_cast<int>(temperature * 63.0);
    const int downfallIndex = static_cast<int>(downfall * 63.0);
    return gBiomeLookup[static_cast<std::size_t>(temperatureIndex + downfallIndex * 64)];
}

BiomeDefinition& Biomes::rainforest() { init(); return gRainforest; }
BiomeDefinition& Biomes::swampland() { init(); return gSwampland; }
BiomeDefinition& Biomes::seasonalForest() { init(); return gSeasonalForest; }
BiomeDefinition& Biomes::forest() { init(); return gForest; }
BiomeDefinition& Biomes::savanna() { init(); return gSavanna; }
BiomeDefinition& Biomes::shrubland() { init(); return gShrubland; }
BiomeDefinition& Biomes::taiga() { init(); return gTaiga; }
BiomeDefinition& Biomes::desert() { init(); return gDesert; }
BiomeDefinition& Biomes::plains() { init(); return gPlains; }
BiomeDefinition& Biomes::iceDesert() { init(); return gIceDesert; }
BiomeDefinition& Biomes::tundra() { init(); return gTundra; }
BiomeDefinition& Biomes::hell() { init(); return gHell; }
BiomeDefinition& Biomes::sky() { init(); return gSky; }

BiomeDefinition& Biomes::byId(BiomeId id)
{
    init();
    switch (id) {
    case BiomeId::Rainforest:
        return gRainforest;
    case BiomeId::Swampland:
        return gSwampland;
    case BiomeId::SeasonalForest:
        return gSeasonalForest;
    case BiomeId::Forest:
        return gForest;
    case BiomeId::Savanna:
        return gSavanna;
    case BiomeId::Shrubland:
        return gShrubland;
    case BiomeId::Taiga:
        return gTaiga;
    case BiomeId::Desert:
        return gDesert;
    case BiomeId::Plains:
        return gPlains;
    case BiomeId::IceDesert:
        return gIceDesert;
    case BiomeId::Tundra:
        return gTundra;
    case BiomeId::Hell:
        return gHell;
    case BiomeId::Sky:
        return gSky;
    }
    return gPlains;
}

} // namespace net::minecraft
