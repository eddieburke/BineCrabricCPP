#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft {
class BlockView;
}

namespace net::minecraft::block {
class LiquidBlock : public Block {
   public:
    static float getFluidHeightFromMeta(int meta);
    static double getFlowingAngle(const BlockView* view, int x, int y, int z, material::Material& material);

   public:
    LiquidBlock(int id, Material& mat) : Block(id, (&mat == &material::Material::LAVA ? 14 : 12) * 16 + 13, mat) {
        const float offsetY = 0.0f;
        const float offsetXZ = 0.0f;
        setBoundingBox(
            0.0f + offsetXZ, 0.0f + offsetY, 0.0f + offsetXZ, 1.0f + offsetXZ, 1.0f + offsetY, 1.0f + offsetXZ);
        setTickRandomly(true);
    }

    [[nodiscard]] bool isFullCube() const override {
        return false;
    }

    [[nodiscard]] bool isOpaque() const override {
        return false;
    }

    [[nodiscard]] int getTexture(int side) const override;
    [[nodiscard]] bool isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const override;
    [[nodiscard]] float getLuminance(const BlockView* blockView, int x, int y, int z) const override;

    [[nodiscard]] int getColorMultiplier(const BlockView* /*blockView*/,
                                         int /*x*/,
                                         int /*y*/,
                                         int /*z*/) const override {
        return 0xFFFFFF;
    }

    [[nodiscard]] int getRenderLayer() const override {
        return &material == &material::Material::WATER ? 1 : 0;
    }

    [[nodiscard]] int getRenderType() const override {
        return 4;
    }

    [[nodiscard]] bool hasCollision(int meta, bool allowLiquids) const override {
        return allowLiquids && meta == 0;
    }

    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override {
        return 0;
    }

    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override {
        return 0;
    }

    [[nodiscard]] int getTickRate() const override;
    void applyVelocity(
        World* world, int x, int y, int z, net::minecraft::Entity* entity, net::minecraft::Vec3d& velocity) override;
    void onPlaced(World* world, int x, int y, int z) override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    void randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) override;

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/,
                                                                       int /*x*/,
                                                                       int /*y*/,
                                                                       int /*z*/) const override {
        return std::nullopt;
    }

   protected:
    [[nodiscard]] int getLiquidState(World* world, int x, int y, int z) const;
    void fizz(World* world, int x, int y, int z);
    void checkBlockCollisions(World* world, int x, int y, int z);
};
}  // namespace net::minecraft::block
