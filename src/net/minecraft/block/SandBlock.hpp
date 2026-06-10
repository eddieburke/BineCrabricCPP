#pragma once

#include "net/minecraft/block/FallingBlock.hpp"

namespace net::minecraft::block {

class SandBlock : public FallingBlock {
public:
    static void registerClass();
    SandBlock(int id, int textureId) : FallingBlock(id, textureId, material::Material::SAND) {}
};

} // namespace net::minecraft::block
