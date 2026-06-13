#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"

namespace net::minecraft {

// Faithful 1:1 port of net.minecraft.world.gen.feature.NetherFirePatchFeature.
class NetherFirePatchFeature : public Feature {
public:
    bool generate(World* world, JavaRandom& random, int x, int y, int z) override
    {
        for (int i = 0; i < 64; ++i) {
            const int nxOffset = random.nextInt(8);
            const int nx = x + nxOffset - random.nextInt(8);
            const int nyOffset = random.nextInt(4);
            const int ny = y + nyOffset - random.nextInt(4);
            const int nzOffset = random.nextInt(8);
            const int nz = z + nzOffset - random.nextInt(8);
            if (!world->isAir(nx, ny, nz) || world->getBlockId(nx, ny - 1, nz) != Block::NETHERRACK->id) {
                continue;
            }
            world->setBlock(nx, ny, nz, Block::FIRE->id);
        }
        return true;
    }
};

} // namespace net::minecraft
