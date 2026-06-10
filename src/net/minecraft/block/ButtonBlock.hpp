#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::block {

class ButtonBlock : public Block {
public:
    using Block::canPlaceAt;
    ButtonBlock(int id, int textureId);

    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getTickRate() const override { return 20; }
    [[nodiscard]] bool canEmitRedstonePower() const override { return true; }
    [[nodiscard]] bool isEmittingRedstonePowerInDirection(
        const BlockView* blockView, int x, int y, int z, int /*direction*/) const override;
    [[nodiscard]] bool canTransferPowerInDirection(
        World* world, int x, int y, int z, int direction) const override;
    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(
        World* /*world*/, int /*x*/, int /*y*/, int /*z*/) const override
    {
        return std::nullopt;
    }
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z, int side) const override;

    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
    [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override;
    void onPlaced(World* world, int x, int y, int z, int direction) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    void onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void onBreak(World* world, int x, int y, int z) override;
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;

private:
    [[nodiscard]] int getPlacementSide(World* world, int x, int y, int z) const;
    bool breakIfCannotPlaceAt(World* world, int x, int y, int z);
    void notifyAttachedNeighbors(World* world, int x, int y, int z, int meta);
};

} // namespace net::minecraft::block
