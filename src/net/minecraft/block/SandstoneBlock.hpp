#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::block {

class SandstoneBlock : public Block {
public:
    static void registerClass();
    explicit SandstoneBlock(int id) : Block(id, 192, material::Material::STONE) {}

    [[nodiscard]] int getTexture(int side) const override
    {
        return Block::textureForSide(side, textureId, textureId + 16, textureId - 16);
    }
};

} // namespace net::minecraft::block