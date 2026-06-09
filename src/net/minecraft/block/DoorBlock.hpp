#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {

class DoorBlock : public Block {
public:
    DoorBlock(int id, Material& material);

    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getRenderType() const override { return 7; }
    [[nodiscard]] int getTexture(int side, int meta) const override;
    using Block::canPlaceAt;
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
    [[nodiscard]] int getDroppedItemId(int blockMeta, JavaRandom& random) const override;
    [[nodiscard]] int getPistonBehavior() const { return 1; }

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* world, int x, int y, int z) const override;
    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;

    void onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    [[nodiscard]] std::optional<net::minecraft::HitResult> raycast(
        net::minecraft::World* world, int x, int y, int z, net::minecraft::Vec3d startPos, net::minecraft::Vec3d endPos) const override;

    void setOpen(World* world, int x, int y, int z, bool open);
    [[nodiscard]] static bool getOpen(int meta) { return (meta & 4) != 0; }
    [[nodiscard]] static int facingFromMeta(int meta);

private:
    [[nodiscard]] static int getState(const BlockView* blockView, int x, int y, int z);
    [[nodiscard]] static int getState(const World* world, int x, int y, int z);
    [[nodiscard]] static std::optional<net::minecraft::Box> collisionShapeForState(int state, int x, int y, int z);
    void applyBoundsForState(int state);
};

} // namespace net::minecraft::block
