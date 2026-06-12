#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {

class PistonHeadBlock : public Block {
public:
    static constexpr bool kRegisters = true;
    static constexpr int kBlockId = 34;

static void registerClass();
    using Block::canPlaceAt;

    PistonHeadBlock(int id, int textureId);

    [[nodiscard]] int getRenderType() const override { return 17; }
    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 0; }
    [[nodiscard]] int getTexture(int side, int meta) const override;

    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z, int side) const override;
    void onBreak(World* world, int x, int y, int z) override;
    void addIntersectingBoundingBox(
        World* world, int x, int y, int z, const net::minecraft::Box& box, std::vector<Box>& boxes) const override;
    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
    [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;

    static int getFacing(int meta) { return meta & 7; }
};

} // namespace net::minecraft::block
