#pragma once

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::block {

class BookshelfBlock : public Block {
public:
    BookshelfBlock(int id, int textureId) : Block(id, textureId, material::Material::WOOD) {}

    [[nodiscard]] int getTexture(int side) const override
    {
        if (side <= 1) {
            return 4;
        }
        return textureId;
    }

    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 0; }
};

} // namespace net::minecraft::block