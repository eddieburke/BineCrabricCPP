#pragma once

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::block {

class StoneBlock : public Block {
public:
    static void registerClass();
    static void registerSmeltingRecipes();
    StoneBlock(int id, int textureId) : Block(id, textureId, material::Material::STONE) {}
    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override { return 4; }
};

} // namespace net::minecraft::block