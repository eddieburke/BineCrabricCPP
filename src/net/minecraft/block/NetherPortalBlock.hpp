#pragma once

#include "net/minecraft/block/TranslucentBlock.hpp"

namespace net::minecraft::block {

class NetherPortalBlock : public TranslucentBlock {
public:
    static constexpr int kBlockId = 90;

static void registerClass();
    NetherPortalBlock(int id, int textureId) : TranslucentBlock(id, textureId, material::Material::NETHER_PORTAL, false) {}

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(
        World* world, int x, int y, int z) const override;
    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
    [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override;

    bool create(World* world, int x, int y, int z);
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    [[nodiscard]] bool isSideVisible(
        const BlockView* blockView, int x, int y, int z, int side) const override;
    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 0; }
    [[nodiscard]] int getRenderLayer() const override { return 1; }
    void onEntityCollision(
        World* world, int x, int y, int z, net::minecraft::Entity* entity) override;
    void randomDisplayTick(
        World* world, int x, int y, int z, JavaRandom& random) override;
};

} // namespace net::minecraft::block
