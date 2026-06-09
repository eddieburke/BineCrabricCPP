#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {

class TrapdoorBlock : public Block {
public:
    using Block::canPlaceAt;
    TrapdoorBlock(int id, Material& material);

    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getRenderType() const override { return 0; }
    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z, int side) const override;

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* world, int x, int y, int z) const override;
    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
    void setupRenderBoundingBox() override;
    [[nodiscard]] std::optional<net::minecraft::HitResult> raycast(
        World* world, int x, int y, int z, net::minecraft::Vec3d startPos, net::minecraft::Vec3d endPos) const override;

    void onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    void onPlaced(World* world, int x, int y, int z, int direction) override;

    void setOpen(World* world, int x, int y, int z, bool open);
    [[nodiscard]] static bool isOpen(int meta) { return (meta & 4) != 0; }

private:
    void applyBoundsForMeta(int meta);
};

} // namespace net::minecraft::block
