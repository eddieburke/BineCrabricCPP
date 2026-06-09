#pragma once

#include "net/minecraft/block/PlantBlock.hpp"
#include "net/minecraft/client/color/world/GrassColors.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

#include <cstdint>
#include <vector>

namespace net::minecraft::block {

class TallPlantBlock : public PlantBlock {
public:
    TallPlantBlock(int id, int textureId) : PlantBlock(id, textureId)
    {
        const float f = 0.4f;
        setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.8f, 0.5f + f);
    }

    [[nodiscard]] int getTexture(int /*side*/, int meta) const override
    {
        if (meta == 1) {
            return textureId;
        }
        if (meta == 2) {
            return textureId + 16 + 1;
        }
        if (meta == 0) {
            return textureId + 16;
        }
        return textureId;
    }

    [[nodiscard]] int getColorMultiplier(const BlockView* blockView, int x, int y, int z) const override
    {
        if (blockView == nullptr) {
            return 0xFFFFFF;
        }
        const int meta = blockView->getBlockMeta(x, y, z);
        if (meta == 0) {
            return 0xFFFFFF;
        }
        std::int64_t seed = static_cast<std::int64_t>(x) * 3129871 + static_cast<std::int64_t>(z) * 6129781 + y;
        seed = seed * seed * 42317861LL + seed * 11LL;
        x = static_cast<int>(static_cast<std::int64_t>(x) + (seed >> 14 & 0x1FL));
        y = static_cast<int>(static_cast<std::int64_t>(y) + (seed >> 19 & 0x1FL));
        z = static_cast<int>(static_cast<std::int64_t>(z) + (seed >> 24 & 0x1FL));
        net::minecraft::BiomeSource* biomeSource = blockView->getBiomeSource();
        if (biomeSource == nullptr) {
            return 0xFFFFFF;
        }
        std::vector<net::minecraft::BiomeInfo> scratch;
        biomeSource->getBiomesInArea(scratch, x, z, 1, 1);
        const auto& temperatureMap = biomeSource->temperatureMap();
        const auto& downfallMap = biomeSource->downfallMap();
        if (temperatureMap.empty() || downfallMap.empty()) {
            return 0xFFFFFF;
        }
        return net::minecraft::client::color::world::GrassColors::getColor(temperatureMap[0], downfallMap[0]);
    }

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& random) const override
    {
        if (random.nextInt(8) == 0) {
            return 295; // Item.SEEDS
        }
        return -1;
    }
};

} // namespace net::minecraft::block
