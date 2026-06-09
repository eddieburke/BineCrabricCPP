#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace net::minecraft {

enum class BiomeId {
    Rainforest,
    Swampland,
    SeasonalForest,
    Forest,
    Savanna,
    Shrubland,
    Taiga,
    Desert,
    Plains,
    IceDesert,
    Tundra,
    Hell,
    Sky,
};

struct BiomeInfo {
    BiomeId id = BiomeId::Plains;
    std::string_view name = "Plains";
    std::uint8_t topBlockId = 2;
    std::uint8_t soilBlockId = 3;

    [[nodiscard]] int getSkyColor(float f) const
    {
        if ((f /= 3.0f) < -1.0f) {
            f = -1.0f;
        }
        if (f > 1.0f) {
            f = 1.0f;
        }

        const float hue = 0.62222224f - f * 0.05f;
        const float saturation = 0.5f + f * 0.1f;
        const float brightness = 1.0f;

        const float wrappedHue = hue - std::floor(hue);
        const float h = wrappedHue * 6.0f;
        const int sector = static_cast<int>(std::floor(h));
        const float fraction = h - static_cast<float>(sector);
        const float p = brightness * (1.0f - saturation);
        const float q = brightness * (1.0f - saturation * fraction);
        const float t = brightness * (1.0f - saturation * (1.0f - fraction));

        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;
        switch (sector) {
        case 0:
            red = brightness;
            green = t;
            blue = p;
            break;
        case 1:
            red = q;
            green = brightness;
            blue = p;
            break;
        case 2:
            red = p;
            green = brightness;
            blue = t;
            break;
        case 3:
            red = p;
            green = q;
            blue = brightness;
            break;
        case 4:
            red = t;
            green = p;
            blue = brightness;
            break;
        default:
            red = brightness;
            green = p;
            blue = q;
            break;
        }

        const int r = static_cast<int>(red * 255.0f) & 0xFF;
        const int g = static_cast<int>(green * 255.0f) & 0xFF;
        const int b = static_cast<int>(blue * 255.0f) & 0xFF;
        return (r << 16) | (g << 8) | b;
    }
};

inline BiomeInfo locateBiome(double temperature, double downfall)
{
    downfall *= temperature;
    if (temperature < 0.1) {
        return {BiomeId::Tundra, "Tundra", 2, 3};
    }
    if (downfall < 0.2) {
        if (temperature < 0.5) {
            return {BiomeId::Tundra, "Tundra", 2, 3};
        }
        if (temperature < 0.95) {
            return {BiomeId::Savanna, "Savanna", 2, 3};
        }
        return {BiomeId::Desert, "Desert", 12, 12};
    }
    if (downfall > 0.5 && temperature < 0.7) {
        return {BiomeId::Swampland, "Swampland", 2, 3};
    }
    if (temperature < 0.5) {
        return {BiomeId::Taiga, "Taiga", 2, 3};
    }
    if (temperature < 0.97) {
        if (downfall < 0.35) {
            return {BiomeId::Shrubland, "Shrubland", 2, 3};
        }
        return {BiomeId::Forest, "Forest", 2, 3};
    }
    if (downfall < 0.45) {
        return {BiomeId::Plains, "Plains", 2, 3};
    }
    if (downfall < 0.9) {
        return {BiomeId::SeasonalForest, "Seasonal Forest", 2, 3};
    }
    return {BiomeId::Rainforest, "Rainforest", 2, 3};
}

} // namespace net::minecraft
