#pragma once

#include "net/minecraft/block/TransparentBlock.hpp"
#include "net/minecraft/client/color/world/FoliageColors.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

#include <vector>

namespace net::minecraft::block {

class LeavesBlock : public TransparentBlock {
public:
    int spriteIndex = 0;

    LeavesBlock(int id, int textureId);

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override;
    [[nodiscard]] int getDroppedItemCount(JavaRandom& random) const override;
    [[nodiscard]] bool isOpaque() const override { return !renderSides; }
    [[nodiscard]] int getTexture(int /*side*/, int meta) const override;
    [[nodiscard]] int getColor(int meta) const override;
    [[nodiscard]] int getColorMultiplier(const BlockView* blockView, int x, int y, int z) const override;
    void setFancyGraphics(bool fancy);
    void onBreak(World* world, int x, int y, int z) override;
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
    void afterBreak(
        World* world,
        net::minecraft::PlayerEntity* player,
        int x,
        int y,
        int z,
        int meta) override;

protected:
    [[nodiscard]] int getDroppedItemMeta(int blockMeta) const override { return blockMeta & 3; }

private:
    void breakLeaves(World* world, int x, int y, int z);
    std::vector<int> decayRegion_;
};

} // namespace net::minecraft::block
