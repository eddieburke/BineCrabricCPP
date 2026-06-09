#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"

namespace net::minecraft {

// Faithful 1:1 port of net.minecraft.world.gen.feature.PlantPatchFeature.
class PlantPatchFeature : public Feature {
public:
    explicit PlantPatchFeature(int blockId) : blockId_(blockId) {}

    bool generate(World* world, JavaRandom& random, int x, int y, int z) override
    {
        for (int i = 0; i < 64; ++i) {
            const int nx = x + random.nextInt(8) - random.nextInt(8);
            const int ny = y + random.nextInt(4) - random.nextInt(4);
            const int nz = z + random.nextInt(8) - random.nextInt(8);
            if (!world->isAir(nx, ny, nz) || !Block::BLOCKS[static_cast<std::size_t>(blockId_)]->canGrow(world, nx, ny, nz)) {
                continue;
            }
            world->setBlockWithoutNotifyingNeighbors(nx, ny, nz, blockId_);
        }
        return true;
    }

private:
    int blockId_ = 0;
};

} // namespace net::minecraft
