#pragma once

#include "net/minecraft/block/TranslucentBlock.hpp"

namespace net::minecraft::block {

class GlassBlock : public TranslucentBlock {
public:
    static void registerClass();
    static void registerSmeltingRecipes();
    GlassBlock(int id, int textureId, Material& material, bool transparent)
        : TranslucentBlock(id, textureId, material, transparent) {}

    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 0; }
    [[nodiscard]] int getRenderLayer() const override { return 0; }
};

} // namespace net::minecraft::block