#pragma once

#include "net/minecraft/block/FallingBlock.hpp"

namespace net::minecraft::block {

// Registered in Block.cpp.
class SandBlock : public FallingBlock {
public:
    SandBlock(int id, int textureId) : FallingBlock(id, textureId, material::Material::SAND) {}
};

} // namespace net::minecraft::block
