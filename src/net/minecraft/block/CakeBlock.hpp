#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {

class CakeBlock : public Block {
public:
    using Block::canPlaceAt;
    CakeBlock(int id, int textureId) : Block(id, textureId, material::Material::CAKE)
    {
        setTickRandomly(true);
    }

    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override { return 0; }
    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 0; }

    [[nodiscard]] int getTexture(int side, int meta) const override
    {
        if (side == 1) {
            return textureId;
        }
        if (side == 0) {
            return textureId + 3;
        }
        if (meta > 0 && side == 4) {
            return textureId + 2;
        }
        return textureId + 1;
    }

    [[nodiscard]] int getTexture(int side) const override
    {
        if (side == 1) {
            return textureId;
        }
        if (side == 0) {
            return textureId + 3;
        }
        return textureId + 1;
    }

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* world, int x, int y, int z) const override
    {
        const int bites = world != nullptr ? world->getBlockMeta(x, y, z) : 0;
        const double minX = static_cast<double>(1 + bites * 2) / 16.0;
        constexpr double inset = 0.0625;
        constexpr double height = 0.5;
        return net::minecraft::Box {
            static_cast<double>(x) + minX,
            static_cast<double>(y),
            static_cast<double>(z) + inset,
            static_cast<double>(x + 1) - inset,
            static_cast<double>(y) + height - inset,
            static_cast<double>(z + 1) - inset,
        };
    }

    void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override
    {
        setBoundingBox(getRenderBounds(blockView, x, y, z));
    }

    [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override
    {
        const int bites = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
        const float boundsMinX = static_cast<float>(1 + bites * 2) / 16.0f;
        return {boundsMinX, 0.0f, 0.0625f, 0.9375f, 0.5f, 0.9375f};
    }

    void setupRenderBoundingBox() override
    {
        setBoundingBox(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.5f, 0.9375f);
    }

    [[nodiscard]] bool canGrow(World* world, int x, int y, int z) const override
    {
        return world != nullptr && world->getMaterial(x, y - 1, z).isSolid();
    }

    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const
    {
        return Block::canPlaceAt(world, x, y, z) && canGrow(world, x, y, z);
    }

    void neighborUpdate(World* world, int x, int y, int z, int /*id*/) override
    {
        if (world == nullptr || canGrow(world, x, y, z)) {
            return;
        }
        dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, 0);
    }
};

} // namespace net::minecraft::block
