#pragma once

#include "net/minecraft/block/FallingBlock.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/misc/flint.hpp"

namespace net::minecraft::block {

// Registered in Block.cpp.
class GravelBlock : public FallingBlock {
public:
    GravelBlock(int id, int textureId) : FallingBlock(id, textureId, material::Material::SAND) {}

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& random) const override
    {
        if (random.nextInt(10) == 0) {
            return Item::byRawId(62) != nullptr ? Item::byRawId(62)->id : 318;
        }
        return id;
    }
};

} // namespace net::minecraft::block
