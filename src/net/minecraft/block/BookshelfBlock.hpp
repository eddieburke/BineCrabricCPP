#pragma once

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::block {

class BookshelfBlock : public Block {
public:
    BookshelfBlock(int id, int textureId) : Block(id, textureId, material::Material::WOOD) {}

    [[nodiscard]] int getTexture(int side) const override
    {
        return Block::textureForSide(side, textureId, 4, 4);
    }

    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 0; }
};

} // namespace net::minecraft::block