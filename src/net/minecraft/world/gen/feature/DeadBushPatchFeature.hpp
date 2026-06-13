#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"

namespace net::minecraft {

// Faithful 1:1 port of net.minecraft.world.gen.feature.DeadBushPatchFeature.
class DeadBushPatchFeature : public Feature {
public:
    explicit DeadBushPatchFeature(int deadBushBlockId) : deadBushBlockId_(deadBushBlockId) {}

    bool generate(World* world, JavaRandom& random, int x, int y, int z) override
    {
        int n = 0;
        while (((n = world->getBlockId(x, y, z)) == 0 || n == Block::LEAVES->id) && y > 0) {
            --y;
        }
        for (int i = 0; i < 4; ++i) {
            const int nxOffset = random.nextInt(8);
            const int nx = x + nxOffset - random.nextInt(8);
            const int nyOffset = random.nextInt(4);
            const int ny = y + nyOffset - random.nextInt(4);
            const int nzOffset = random.nextInt(8);
            const int nz = z + nzOffset - random.nextInt(8);
            if (!world->isAir(nx, ny, nz) || !Block::BLOCKS[static_cast<std::size_t>(deadBushBlockId_)]->canGrow(world, nx, ny, nz)) {
                continue;
            }
            world->setBlockWithoutNotifyingNeighbors(nx, ny, nz, deadBushBlockId_);
        }
        return true;
    }

private:
    int deadBushBlockId_ = 0;
};

} // namespace net::minecraft
