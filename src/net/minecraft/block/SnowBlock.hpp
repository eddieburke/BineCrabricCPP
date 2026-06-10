#pragma once

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::block {

class SnowBlock : public Block {
public:
    static void registerClass();
    SnowBlock(int id, int textureId);

    [[nodiscard]] int getDroppedItemId(int blockMeta, JavaRandom& random) const override;
    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 4; }
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
};

} // namespace net::minecraft::block
