#pragma once

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::block {

class OreStorageBlock : public Block {
public:
    static void registerClass();
    OreStorageBlock(int id, int textureId) : Block(id, textureId, material::Material::METAL) {}

    [[nodiscard]] int getTexture(int /*side*/) const override { return textureId; }
};

} // namespace net::minecraft::block