#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/world/ports/IBlockWorld.hpp"

namespace net::minecraft::block {

// Registered in Block.cpp.
class CactusBlock : public Block {
public:
    using Block::canPlaceAt;
    CactusBlock(int id, int textureId) : Block(id, textureId, material::Material::CACTUS)
    {
        setTickRandomly(true);
    }

    void onTick(World* world, int x, int y, int z, JavaRandom& /*random*/) override
    {
        if (!world->isAir(x, y + 1, z)) {
            return;
        }
        int height = 1;
        while (world->getBlockId(x, y - height, z) == id) {
            ++height;
        }
        if (height >= 3) {
            return;
        }
        const int meta = world->getBlockMeta(x, y, z);
        if (meta == 15) {
            world->setBlock(x, y + 1, z, id);
            world->setBlockMeta(x, y, z, 0);
        } else {
            world->setBlockMeta(x, y, z, meta + 1);
        }
    }

    [[nodiscard]] int getTexture(int side) const override
    {
        return Block::textureForSide(side, textureId, textureId + 1, textureId - 1);
    }

    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] int getRenderType() const override { return 13; }

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/, int x, int y, int z) const override
    {
        constexpr double inset = 0.0625;
        return net::minecraft::Box {
            static_cast<double>(x) + inset,
            static_cast<double>(y),
            static_cast<double>(z) + inset,
            static_cast<double>(x + 1) - inset,
            static_cast<double>(y + 1),
            static_cast<double>(z + 1) - inset,
        };
    }

    void setupRenderBoundingBox() override
    {
        setBoundingBox(0.0625f, 0.0f, 0.0625f, 0.9375f, 1.0f, 0.9375f);
    }

    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const
    {
        return Block::canPlaceAt(world, x, y, z) && canGrow(world, x, y, z);
    }

    void neighborUpdate(World* world, int x, int y, int z, int /*id*/) override
    {
        if (world == nullptr) {
            return;
        }
        if (!canGrow(world, x, y, z)) {
            dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
            world->setBlock(x, y, z, 0);
        }
    }

    [[nodiscard]] bool canGrow(World* world, int x, int y, int z) const override
    {
        if (world->getMaterial(x - 1, y, z).isSolid()
            || world->getMaterial(x + 1, y, z).isSolid()
            || world->getMaterial(x, y, z - 1).isSolid()
            || world->getMaterial(x, y, z + 1).isSolid()) {
            return false;
        }
        const int belowId = world->getBlockId(x, y - 1, z);
        return belowId == id || belowId == Block::SAND->id;
    }

    void onEntityCollision(World* world, int x, int y, int z, net::minecraft::Entity* entity) override
    {
        if (entity != nullptr) {
            entity->damage(nullptr, 1);
        }
    }
};

} // namespace net::minecraft::block
