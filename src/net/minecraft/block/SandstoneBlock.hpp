#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {

class SandstoneBlock : public Block {
public:
    explicit SandstoneBlock(int id) : Block(id, 192, material::Material::STONE) {}

    [[nodiscard]] int getTexture(int side) const override
    {
        if (side == 1) {
            return textureId - 16;
        }
        if (side == 0) {
            return textureId + 16;
        }
        return textureId;
    }
};

} // namespace net::minecraft::block