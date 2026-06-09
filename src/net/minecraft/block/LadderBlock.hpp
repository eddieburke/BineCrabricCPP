#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {

class LadderBlock : public Block {
public:
    using Block::canPlaceAt;
    LadderBlock(int id, int textureId);

    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getRenderType() const override { return 8; }
    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 1; }
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* world, int x, int y, int z) const override;
    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
    void onPlaced(World* world, int x, int y, int z, int direction) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;

private:
    void applyBoundsForMeta(int meta);
    [[nodiscard]] static std::optional<net::minecraft::Box> collisionShapeForMeta(int meta, int x, int y, int z);
};

} // namespace net::minecraft::block
