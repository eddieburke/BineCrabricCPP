#pragma once
#include <vector>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/color/world/GrassColors.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

namespace net::minecraft::block {
class GrassBlock : public Block {
   public:
    static constexpr int kBlockId = 2;
    static void registerClass();
    explicit GrassBlock(int id);
    [[nodiscard]] int getDroppedItemId(int blockMeta, JavaRandom& random) const override;
    [[nodiscard]] int getTextureId(const BlockView* blockView, int x, int y, int z, int side) const override;
    [[nodiscard]] int getColorMultiplier(const BlockView* blockView, int x, int y, int z) const override;
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
};
}  // namespace net::minecraft::block
