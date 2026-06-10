#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {

class LogBlock : public Block {
public:
    static void registerClass();
    static void registerBlockItems();
    explicit LogBlock(int id);

    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 1; }
    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override;
    [[nodiscard]] int getTexture(int side, int meta) const override;
    void onBreak(World* world, int x, int y, int z) override;

protected:
    [[nodiscard]] int getDroppedItemMeta(int blockMeta) const override { return blockMeta; }
};

} // namespace net::minecraft::block
