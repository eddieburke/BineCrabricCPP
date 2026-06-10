#pragma once

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::block {

class NetherrackBlock : public Block {
public:
    static void registerClass();
    NetherrackBlock(int id, int textureId) : Block(id, textureId, material::Material::STONE) {}
};

} // namespace net::minecraft::block