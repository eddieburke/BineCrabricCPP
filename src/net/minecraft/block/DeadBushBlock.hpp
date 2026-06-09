#pragma once

#include "net/minecraft/block/PlantBlock.hpp"

namespace net::minecraft::block {

class DeadBushBlock : public PlantBlock {
public:
    DeadBushBlock(int id, int textureId) : PlantBlock(id, textureId)
    {
        const float f = 0.4f;
        setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.8f, 0.5f + f);
    }

    [[nodiscard]] int getTexture(int /*side*/, int /*meta*/) const override { return textureId; }

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override { return -1; }

protected:
    [[nodiscard]] bool canPlantOnTop(int belowId) const override
    {
        return belowId == Block::SAND->id;
    }
};

} // namespace net::minecraft::block
