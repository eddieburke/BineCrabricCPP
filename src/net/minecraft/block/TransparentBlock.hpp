#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::block {

class TransparentBlock : public Block {
public:
    bool renderSides = false;

public:
    TransparentBlock(int id, int textureId, Material& material, bool renderSides)
        : Block(id, textureId, material)
    {
        this->renderSides = renderSides;
    }

    [[nodiscard]] bool isOpaque() const override { return false; }

    [[nodiscard]] bool isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const override
    {
        if (blockView != nullptr && !renderSides && blockView->getBlockId(x, y, z) == id) {
            return false;
        }
        return Block::isSideVisible(blockView, x, y, z, side);
    }
};

} // namespace net::minecraft::block