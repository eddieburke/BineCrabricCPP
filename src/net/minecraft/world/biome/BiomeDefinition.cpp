#include "net/minecraft/world/biome/BiomeDefinition.hpp"

#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/LargeOakTreeFeature.hpp"
#include "net/minecraft/world/gen/feature/OakTreeFeature.hpp"

#include <algorithm>
#include <cmath>

namespace net::minecraft {

BiomeDefinition::BiomeDefinition()
{
    topBlockId = 2;
    soilBlockId = 3;
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

std::unique_ptr<Feature> BiomeDefinition::getRandomTreeFeature(JavaRandom& random)
{
    if (random.nextInt(10) == 0) {
        return std::make_unique<LargeOakTreeFeature>();
    }
    return std::make_unique<OakTreeFeature>();
}

int BiomeDefinition::getSkyColor(float brightness) const
{
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

const std::vector<EntitySpawnGroup>& BiomeDefinition::getSpawnableEntities(int group) const
{
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

} // namespace net::minecraft
