#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <unordered_set>
#include <vector>

namespace net::minecraft {
class BlockView;
class World;
}

namespace net::minecraft::block {

class RedstoneWireBlock : public Block {
public:
    static void registerClass();
    RedstoneWireBlock(int id, int textureId);

    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getRenderType() const override { return 5; }
    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/, int /*x*/, int /*y*/, int /*z*/) const override
    {
        return std::nullopt;
    }
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
    using Block::canPlaceAt;
    [[nodiscard]] bool canEmitRedstonePower() const override { return powered_; }
    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override;

    void onPlaced(World* world, int x, int y, int z) override;
    void onBreak(World* world, int x, int y, int z) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    [[nodiscard]] bool canTransferPowerInDirection(World* world, int x, int y, int z, int direction) const override;
    [[nodiscard]] bool isEmittingRedstonePowerInDirection(const BlockView* blockView, int x, int y, int z, int direction) const override;

    void randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) override;

    static bool shouldConnectTo(const BlockView* blockView, int x, int y, int z, int side);

private:
    void updatePower(World* world, int x, int y, int z);
    void doUpdatePower(World* world, int x, int y, int z, int sourceX, int sourceY, int sourceZ);
    void updateNeighborsOfWire(World* world, int x, int y, int z);
    [[nodiscard]] int getHighestPowerWire(World* world, int x, int y, int z, int power) const;

    bool powered_ = true;
    std::unordered_set<Vec3i, Vec3iHash> neighborsToUpdate_;
};

} // namespace net::minecraft::block
