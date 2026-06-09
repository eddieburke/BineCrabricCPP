#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/color/world/GrassColors.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

#include <vector>

namespace net::minecraft::block {

class GrassBlock : public Block {
public:
    explicit GrassBlock(int id);

    [[nodiscard]] int getDroppedItemId(int blockMeta, JavaRandom& random) const override;
    [[nodiscard]] int getTextureId(const BlockView* blockView, int x, int y, int z, int side) const override;
    [[nodiscard]] int getColorMultiplier(const BlockView* blockView, int x, int y, int z) const override;
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
};

} // namespace net::minecraft::block
