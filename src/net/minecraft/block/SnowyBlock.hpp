#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {

class SnowyBlock : public Block {
public:
    static constexpr bool kRegisters = true;
    static constexpr int kBlockId = 78;

static void registerClass();
    SnowyBlock(int id, int textureId);

    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 0; }
    [[nodiscard]] int getDroppedItemId(int blockMeta, JavaRandom& random) const override;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    void afterBreak(World* world, net::minecraft::PlayerEntity* player, int x, int y, int z, int meta) override;
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
    [[nodiscard]] bool isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const override;

private:
    bool breakIfCannotPlaceAt(World* world, int x, int y, int z);

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* world, int x, int y, int z) const override;

    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override
    {
        setBoundingBox(getRenderBounds(blockView, x, y, z));
    }

    [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override
    {
        const int layers = blockView != nullptr ? (blockView->getBlockMeta(x, y, z) & 7) : 0;
        const float height = static_cast<float>(2 * (1 + layers)) / 16.0f;
        return {0.0f, 0.0f, 0.0f, 1.0f, height, 1.0f};
    }
};

} // namespace net::minecraft::block
