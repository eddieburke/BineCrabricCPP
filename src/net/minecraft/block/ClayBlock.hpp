#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {

class ClayBlock : public Block {
public:
    static void registerClass();
    ClayBlock(int id, int textureId) : Block(id, textureId, material::Material::CLAY) {}

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override
    {
        return Item::CLAY != nullptr ? Item::CLAY->id : 337;
    }

    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 4; }
};

} // namespace net::minecraft::block