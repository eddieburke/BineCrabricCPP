#pragma once

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::block {

// Registered in Block.cpp.
class DirtBlock : public Block {
public:
    DirtBlock(int id, int textureId) : Block(id, textureId, material::Material::SOIL) {}
};

} // namespace net::minecraft::block