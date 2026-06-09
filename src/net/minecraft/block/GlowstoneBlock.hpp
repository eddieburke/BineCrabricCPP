#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {

class GlowstoneBlock : public Block {
public:
    GlowstoneBlock(int id, int textureId, Material& material) : Block(id, textureId, material) {}

    [[nodiscard]] int getDroppedItemCount(JavaRandom& random) const override
    {
        return 2 + random.nextInt(3);
    }

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override
    {
        return Item::GLOWSTONE_DUST != nullptr ? Item::GLOWSTONE_DUST->id : 348;
    }
};

} // namespace net::minecraft::block