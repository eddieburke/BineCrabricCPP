#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

class PlantBlock : public Block {
public:
    using Block::canPlaceAt;
    PlantBlock(int id, int textureId)
        : Block(id, material::Material::PLANT)
    {
        this->textureId = textureId;
        setTickRandomly(true);
        const float f = 0.2f;
        setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 3.0f, 0.5f + f);
    }

    [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const
    {
        return Block::canPlaceAt(world, x, y, z) && canPlantOnTop(world->getBlockId(x, y - 1, z));
    }

    [[nodiscard]] bool isOpaque() const override { return false; }
    [[nodiscard]] bool isFullCube() const override { return false; }
    [[nodiscard]] int getRenderType() const override { return 1; }

    [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* /*world*/, int /*x*/, int /*y*/, int /*z*/) const override
    {
        return std::nullopt;
    }

    void neighborUpdate(World* world, int x, int y, int z, int id) override
    {
        Block::neighborUpdate(world, x, y, z, id);
        if (world == nullptr || world->isRemote()) {
            return;
        }
        breakIfCannotGrow(world, x, y, z);
    }

    void onTick(World* world, int x, int y, int z, JavaRandom& /*random*/) override
    {
        if (world == nullptr || world->isRemote()) {
            return;
        }
        breakIfCannotGrow(world, x, y, z);
    }

    [[nodiscard]] bool canGrow(World* world, int x, int y, int z) const override
    {
        return (world->getBrightness(x, y, z) >= 8 || world->hasSkyLight(x, y, z))
            && canPlantOnTop(world->getBlockId(x, y - 1, z));
    }

protected:
    [[nodiscard]] virtual bool canPlantOnTop(int belowId) const
    {
        constexpr int kGrassBlockId = 2;
        constexpr int kDirtId = 3;
        constexpr int kFarmlandId = 60;
        const int grassBlockId = Block::GRASS_BLOCK != nullptr ? Block::GRASS_BLOCK->id : kGrassBlockId;
        const int dirtId = Block::DIRT != nullptr ? Block::DIRT->id : kDirtId;
        const int farmlandId = Block::FARMLAND != nullptr ? Block::FARMLAND->id : kFarmlandId;
        return belowId == grassBlockId || belowId == dirtId || belowId == farmlandId;
    }

    void breakIfCannotGrow(World* world, int x, int y, int z)
    {
        if (world == nullptr || world->isRemote()) {
            return;
        }
        if (!canGrow(world, x, y, z)) {
            const int meta = world->getBlockMeta(x, y, z);
            dropStacks(world, x, y, z, meta);
            world->setBlock(x, y, z, 0);
        }
    }
};

} // namespace net::minecraft::block
