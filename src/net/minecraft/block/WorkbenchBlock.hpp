#pragma once

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::block {

class WorkbenchBlock : public Block {
public:
    explicit WorkbenchBlock(int id) : Block(id, 59, material::Material::WOOD) {}

    [[nodiscard]] int getTexture(int side) const override;
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
};

} // namespace net::minecraft::block
