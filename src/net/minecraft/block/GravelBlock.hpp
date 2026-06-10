#pragma once

#include "net/minecraft/block/FallingBlock.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {

class GravelBlock : public FallingBlock {
public:
    static void registerClass();
    GravelBlock(int id, int textureId) : FallingBlock(id, textureId, material::Material::SAND) {}

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& random) const override
    {
        if (random.nextInt(10) == 0) {
            return Item::FLINT != nullptr ? Item::FLINT->id : 318;
        }
        return id;
    }
};

} // namespace net::minecraft::block
