#pragma once

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/TreeFeatureHelpers.hpp"

#include <cstdlib>

namespace net::minecraft {

class World;

class PineTreeFeature : public Feature {
public:

    bool generate(World* world, JavaRandom& random, int x, int y, int z) override
    {
        const int height = random.nextInt(5) + 7;
        const int trunkClearHeight = height - random.nextInt(2) - 3;
        const int crownHeight = height - trunkClearHeight;
        const int maxRadius = 1 + random.nextInt(crownHeight + 1);
        bool clear = true;
        if (world == nullptr || y < 1 || y + height + 1 > 128) {
            return false;
        }
        for (int n5 = y; n5 <= y + 1 + height && clear; ++n5) {
            int radius = n5 - y < trunkClearHeight ? 0 : maxRadius;
            for (int n3 = x - radius; n3 <= x + radius && clear; ++n3) {
                for (int n2 = z - radius; n2 <= z + radius && clear; ++n2) {
                    if (n5 >= 0 && n5 < 128) {
                        const int blockId = world->getBlockId(n3, n5, n2);
                        if (tree_feature::canReplaceLeaves(blockId)) {
                            continue;
                        }
                        clear = false;
                        continue;
                    }
                    clear = false;
                }
            }
        }
        if (!clear) {
            return false;
        }
        const int ground = world->getBlockId(x, y - 1, z);
        if (!tree_feature::isGrassOrDirt(ground) || y >= 128 - height - 1) {
            return false;
        }
        world->setBlockWithoutNotifyingNeighbors(x, y - 1, z, tree_feature::id(Block::DIRT));
        int radius = 0;
        for (int n3 = y + height; n3 >= y + trunkClearHeight; --n3) {
            for (int n2 = x - radius; n2 <= x + radius; ++n2) {
                const int dx = n2 - x;
                for (int i = z - radius; i <= z + radius; ++i) {
                    const int dz = i - z;
                    if ((std::abs(dx) == radius && std::abs(dz) == radius && radius > 0)
                        || tree_feature::isOpaqueBlock(world->getBlockId(n2, n3, i))) {
                        continue;
                    }
                    world->setBlockWithoutNotifyingNeighbors(n2, n3, i, tree_feature::id(Block::LEAVES), 1);
                }
            }
            if (radius >= 1 && n3 == y + trunkClearHeight + 1) {
                --radius;
                continue;
            }
            if (radius < maxRadius) {
                ++radius;
            }
        }
        for (int n3 = 0; n3 < height - 1; ++n3) {
            const int blockId = world->getBlockId(x, y + n3, z);
            if (!tree_feature::canReplaceLeaves(blockId)) {
                continue;
            }
            world->setBlockWithoutNotifyingNeighbors(x, y + n3, z, tree_feature::id(Block::LOG), 1);
        }
        return true;
    }
};

} // namespace net::minecraft
