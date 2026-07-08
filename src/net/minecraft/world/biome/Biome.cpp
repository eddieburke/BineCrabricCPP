#include "net/minecraft/world/biome/Biome.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/biome/BasicBiome.hpp"
#include "net/minecraft/world/biome/ForestBiome.hpp"
#include "net/minecraft/world/biome/HellBiome.hpp"
#include "net/minecraft/world/biome/RainforestBiome.hpp"
#include "net/minecraft/world/biome/SkyBiome.hpp"
#include "net/minecraft/world/biome/SwamplandBiome.hpp"
#include "net/minecraft/world/biome/TaigaBiome.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/LargeOakTreeFeature.hpp"
#include "net/minecraft/world/gen/feature/OakTreeFeature.hpp"

namespace net::minecraft {
namespace {
std::array<Biome*, 4096> gBiomeLookup{};
RainforestBiome gRainforest;
SwamplandBiome gSwampland;
Biome gSeasonalForest;
ForestBiome gForest;
BasicBiome gSavanna;
Biome gShrubland;
TaigaBiome gTaiga;
BasicBiome gDesert;
BasicBiome gPlains;
BasicBiome gIceDesert;
Biome gTundra;
HellBiome gHell;
SkyBiome gSky;
bool gInitialized = false;

struct BiomeBootstrap {
    static void registerClass() {
        Biome::init();
    }
};

static registry::RegisterPhase<BiomeBootstrap> s_reg(mod::LifecyclePhase::BiomeRegistration, 0);
}  // namespace

Biome::Biome() {
    topBlockId = kBiomeDefaultTopBlockId;
    soilBlockId = kBiomeDefaultSoilBlockId;
    foliageColor = 5169201;
    hasRain = true;
    spawnableMonsters_.push_back({"Spider", 10});
    spawnableMonsters_.push_back({"Zombie", 10});
    spawnableMonsters_.push_back({"Skeleton", 10});
    spawnableMonsters_.push_back({"Creeper", 10});
    spawnableMonsters_.push_back({"Slime", 10});
    spawnablePassive_.push_back({"Sheep", 12});
    spawnablePassive_.push_back({"Pig", 10});
    spawnablePassive_.push_back({"Chicken", 10});
    spawnablePassive_.push_back({"Cow", 8});
    spawnableWaterCreatures_.push_back({"Squid", 10});
}

std::unique_ptr<Feature> Biome::getRandomTreeFeature(JavaRandom& random) const {
    if (random.nextInt(10) == 0) {
        return std::make_unique<LargeOakTreeFeature>();
    }
    return std::make_unique<OakTreeFeature>();
}

int Biome::getSkyColor(float brightness) const {
    float f = brightness / 3.0f;
    if (f < -1.0f) {
        f = -1.0f;
    }
    if (f > 1.0f) {
        f = 1.0f;
    }
    const float hue = 0.62222224f - f * 0.05f;
    const float saturation = 0.5f + f * 0.1f;
    const float value = 1.0f;
    const float wrappedHue = hue - std::floor(hue);
    const float h = wrappedHue * 6.0f;
    const int sector = static_cast<int>(std::floor(h));
    const float fraction = h - static_cast<float>(sector);
    const float p = value * (1.0f - saturation);
    const float q = value * (1.0f - saturation * fraction);
    const float t = value * (1.0f - saturation * (1.0f - fraction));
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    switch (sector) {
        case 0:
            red = value;
            green = t;
            blue = p;
            break;
        case 1:
            red = q;
            green = value;
            blue = p;
            break;
        case 2:
            red = p;
            green = value;
            blue = t;
            break;
        case 3:
            red = p;
            green = q;
            blue = value;
            break;
        case 4:
            red = t;
            green = p;
            blue = value;
            break;
        default:
            red = value;
            green = p;
            blue = q;
            break;
    }
    const int r = static_cast<int>(red * 255.0f) & 0xFF;
    const int g = static_cast<int>(green * 255.0f) & 0xFF;
    const int b = static_cast<int>(blue * 255.0f) & 0xFF;
    return (r << 16) | (g << 8) | b;
}

const std::vector<EntitySpawnGroup>& Biome::getSpawnableEntities(int group) const {
    if (group == 0) {
        return spawnableMonsters_;
    }
    if (group == 1) {
        return spawnablePassive_;
    }
    if (group == 2) {
        return spawnableWaterCreatures_;
    }
    static const std::vector<EntitySpawnGroup> empty;
    return empty;
}

void Biome::init() {
    if (gInitialized) {
        return;
    }
    gInitialized = true;
    gRainforest.id = BiomeId::Rainforest;
    gRainforest.wireName_ = "rainforest";
    gRainforest.setGrassColor(588342).setName("Rainforest").setFoliageColor(2094168);
    gSwampland.id = BiomeId::Swampland;
    gSwampland.wireName_ = "swampland";
    gSwampland.setGrassColor(522674).setName("Swampland").setFoliageColor(9154376);
    gSeasonalForest.id = BiomeId::SeasonalForest;
    gSeasonalForest.wireName_ = "seasonal_forest";
    gSeasonalForest.setGrassColor(10215459).setName("Seasonal Forest");
    gForest.id = BiomeId::Forest;
    gForest.wireName_ = "forest";
    gForest.setGrassColor(353825).setName("Forest").setFoliageColor(5159473);
    gSavanna.id = BiomeId::Savanna;
    gSavanna.wireName_ = "savanna";
    gSavanna.setGrassColor(14278691).setName("Savanna");
    gShrubland.id = BiomeId::Shrubland;
    gShrubland.wireName_ = "shrubland";
    gShrubland.setGrassColor(10595616).setName("Shrubland");
    gTaiga.id = BiomeId::Taiga;
    gTaiga.wireName_ = "taiga";
    gTaiga.setGrassColor(3060051).setName("Taiga").enableSnow().setFoliageColor(8107825);
    gDesert.id = BiomeId::Desert;
    gDesert.wireName_ = "desert";
    gDesert.setGrassColor(16421912).setName("Desert").disableRain();
    gPlains.id = BiomeId::Plains;
    gPlains.wireName_ = "plains";
    gPlains.setGrassColor(16767248).setName("Plains");
    gIceDesert.id = BiomeId::IceDesert;
    gIceDesert.wireName_ = "ice_desert";
    gIceDesert.setGrassColor(16772499).setName("Ice Desert").enableSnow().disableRain().setFoliageColor(12899129);
    gTundra.id = BiomeId::Tundra;
    gTundra.wireName_ = "tundra";
    gTundra.setGrassColor(5762041).setName("Tundra").enableSnow().setFoliageColor(12899129);
    gHell.id = BiomeId::Hell;
    gHell.wireName_ = "hell";
    gHell.setGrassColor(0xFF0000).setName("Hell").disableRain();
    gSky.id = BiomeId::Sky;
    gSky.wireName_ = "sky";
    gSky.setGrassColor(0x8080FF).setName("Sky").disableRain();
    gDesert.topBlockId = kBiomeSandSurfaceBlockId;
    gDesert.soilBlockId = kBiomeSandSurfaceBlockId;
    gIceDesert.topBlockId = kBiomeSandSurfaceBlockId;
    gIceDesert.soilBlockId = kBiomeSandSurfaceBlockId;
    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 64; ++j) {
            gBiomeLookup[static_cast<std::size_t>(i + j * 64)] =
                &locateBiome(static_cast<float>(i) / 63.0f, static_cast<float>(j) / 63.0f);
        }
    }
}

Biome& Biome::getBiome(double temperature, double downfall) {
    init();
    const int temperatureIndex = static_cast<int>(temperature * 63.0);
    const int downfallIndex = static_cast<int>(downfall * 63.0);
    return *gBiomeLookup[static_cast<std::size_t>(temperatureIndex + downfallIndex * 64)];
}

Biome& Biome::locateBiome(float temperature, float downfall) {
    init();
    downfall *= temperature;
    if (temperature < 0.1f) {
        return gTundra;
    }
    if (downfall < 0.2f) {
        if (temperature < 0.5f) {
            return gTundra;
        }
        if (temperature < 0.95f) {
            return gSavanna;
        }
        return gDesert;
    }
    if (downfall > 0.5f && temperature < 0.7f) {
        return gSwampland;
    }
    if (temperature < 0.5f) {
        return gTaiga;
    }
    if (temperature < 0.97f) {
        if (downfall < 0.35f) {
            return gShrubland;
        }
        return gForest;
    }
    if (downfall < 0.45f) {
        return gPlains;
    }
    if (downfall < 0.9f) {
        return gSeasonalForest;
    }
    return gRainforest;
}

Biome& Biome::rainforest() {
    init();
    return gRainforest;
}

Biome& Biome::swampland() {
    init();
    return gSwampland;
}

Biome& Biome::seasonalForest() {
    init();
    return gSeasonalForest;
}

Biome& Biome::forest() {
    init();
    return gForest;
}

Biome& Biome::savanna() {
    init();
    return gSavanna;
}

Biome& Biome::shrubland() {
    init();
    return gShrubland;
}

Biome& Biome::taiga() {
    init();
    return gTaiga;
}

Biome& Biome::desert() {
    init();
    return gDesert;
}

Biome& Biome::plains() {
    init();
    return gPlains;
}

Biome& Biome::iceDesert() {
    init();
    return gIceDesert;
}

Biome& Biome::tundra() {
    init();
    return gTundra;
}

Biome& Biome::hell() {
    init();
    return gHell;
}

Biome& Biome::sky() {
    init();
    return gSky;
}

Biome& Biome::byId(BiomeId biomeId) {
    init();
    switch (biomeId) {
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
}  // namespace net::minecraft
