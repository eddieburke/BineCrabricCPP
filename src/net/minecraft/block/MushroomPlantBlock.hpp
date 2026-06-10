#pragma once

#include "net/minecraft/block/PlantBlock.hpp"

namespace net::minecraft::block {

class MushroomPlantBlock : public PlantBlock {
public:
    static void registerClass();
    MushroomPlantBlock(int id, int textureId) : PlantBlock(id, textureId)
    {
        const float f = 0.2f;
        setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f);
    }

    void onTick(World* world, int x, int y, int z, JavaRandom& random) override
    {
        if (world == nullptr) {
            return;
        }
        int spreadX = 0;
        int spreadY = 0;
        int spreadZ = 0;
        if (random.nextInt(100) == 0
            && world->isAir(spreadX = x + random.nextInt(3) - 1, spreadY = y + random.nextInt(2) - random.nextInt(2),
                spreadZ = z + random.nextInt(3) - 1)
            && canGrow(world, spreadX, spreadY, spreadZ)) {
            x += random.nextInt(3) - 1;
            z += random.nextInt(3) - 1;
            if (world->isAir(spreadX, spreadY, spreadZ) && canGrow(world, spreadX, spreadY, spreadZ)) {
                world->setBlock(spreadX, spreadY, spreadZ, id);
            }
        }
    }

    [[nodiscard]] bool canGrow(World* world, int x, int y, int z) const override
    {
        if (y < 0 || y >= 128) {
            return false;
        }
        return world->getBrightness(x, y, z) < 13 && canPlantOnTop(world->getBlockId(x, y - 1, z));
    }

protected:
    [[nodiscard]] bool canPlantOnTop(int belowId) const override
    {
        return belowId >= 0 && belowId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(belowId)];
    }
};

} // namespace net::minecraft::block