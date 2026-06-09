#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::block {

class TranslucentBlock : public Block {
public:
    bool transparent = false;

public:
    TranslucentBlock(int id, int textureId, Material& material, bool transparent)
        : Block(id, textureId, material)
    {
        this->transparent = transparent;
    }

    [[nodiscard]] bool isOpaque() const override { return false; }

    [[nodiscard]] bool isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const override
    {
        if (blockView != nullptr && !transparent && blockView->getBlockId(x, y, z) == id) {
            return false;
        }
        return Block::isSideVisible(blockView, x, y, z, side);
    }
};

} // namespace net::minecraft::block