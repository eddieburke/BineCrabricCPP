#pragma once

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::block {

class DirtBlock : public Block {
public:
    static void registerClass();
    DirtBlock(int id, int textureId) : Block(id, textureId, material::Material::SOIL) {}
};

} // namespace net::minecraft::block