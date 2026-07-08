#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"

namespace net::minecraft {
// Faithful 1:1 port of net.minecraft.world.gen.feature.GlowstoneClusterFeature.
class GlowstoneClusterFeature : public Feature {
   public:
    bool generate(World* world, JavaRandom& random, int x, int y, int z) override {
        if (!world->isAir(x, y, z)) {
            return false;
        }
        if (world->getBlockId(x, y + 1, z) != Block::NETHERRACK->id) {
            return false;
        }
        world->setBlock(x, y, z, Block::GLOWSTONE->id);
        for (int i = 0; i < 1500; ++i) {
            const int nxOffset = random.nextInt(8);
            const int nx = x + nxOffset - random.nextInt(8);
            const int ny = y - random.nextInt(12);
            const int nzOffset = random.nextInt(8);
            const int nz = z + nzOffset - random.nextInt(8);
            if (world->getBlockId(nx, ny, nz) != 0) {
                continue;
            }
            int neighborGlow = 0;
            for (int j = 0; j < 6; ++j) {
                int neighborId = 0;
                if (j == 0) {
                    neighborId = world->getBlockId(nx - 1, ny, nz);
                } else if (j == 1) {
                    neighborId = world->getBlockId(nx + 1, ny, nz);
                } else if (j == 2) {
                    neighborId = world->getBlockId(nx, ny - 1, nz);
                } else if (j == 3) {
                    neighborId = world->getBlockId(nx, ny + 1, nz);
                } else if (j == 4) {
                    neighborId = world->getBlockId(nx, ny, nz - 1);
                } else if (j == 5) {
                    neighborId = world->getBlockId(nx, ny, nz + 1);
                }
                if (neighborId != Block::GLOWSTONE->id) {
                    continue;
                }
                ++neighborGlow;
            }
            if (neighborGlow != 1) {
                continue;
            }
            world->setBlock(nx, ny, nz, Block::GLOWSTONE->id);
        }
        return true;
    }
};
}  // namespace net::minecraft
