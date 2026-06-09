#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"

namespace net::minecraft {
class BlockView;
class World;
}

namespace net::minecraft::block {

class RepeaterBlock : public Block {
public:
    static constexpr double RENDER_OFFSET[4] = {-0.0625, 0.0625, 0.1875, 0.3125};

    bool lit = false;

    RepeaterBlock(int id, bool litIn);

    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getRenderType() const override { return 15; }
    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] int getTexture(int side) const override { return getTexture(side, 0); }
    [[nodiscard]] int getTexture(int side, int /*meta*/) const override;
    [[nodiscard]] bool isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const override;
    [[nodiscard]] bool canEmitRedstonePower() const override { return false; }
    using Block::canPlaceAt;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
    [[nodiscard]] bool canGrow(World* world, int x, int y, int z) const override;
    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override;

    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void onPlaced(World* world, int x, int y, int z) override;
    void onPlaced(World* world, int x, int y, int z, net::minecraft::PlayerEntity* placer) override;
    [[nodiscard]] bool canTransferPowerInDirection(World* world, int x, int y, int z, int direction) const override;
    [[nodiscard]] bool isEmittingRedstonePowerInDirection(const BlockView* blockView, int x, int y, int z, int direction) const override;
    void randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) override;

private:
    [[nodiscard]] bool isPowered(World* world, int x, int y, int z, int meta) const;
};

} // namespace net::minecraft::block
