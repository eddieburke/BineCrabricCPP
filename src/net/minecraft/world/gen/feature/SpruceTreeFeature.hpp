#pragma once

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/TreeFeatureHelpers.hpp"

#include <cstdlib>

namespace net::minecraft {

class World;

class SpruceTreeFeature : public Feature {
public:

    bool generate(World* world, JavaRandom& random, int x, int y, int z) override
    {
        const int height = random.nextInt(4) + 6;
        const int trunkClearHeight = 1 + random.nextInt(2);
        const int crownHeight = height - trunkClearHeight;
        const int maxRadius = 2 + random.nextInt(2);
        bool clear = true;
        if (world == nullptr || y < 1 || y + height + 1 > 128) {
            return false;
        }
        for (int n7 = y; n7 <= y + 1 + height && clear; ++n7) {
            int radius = n7 - y < trunkClearHeight ? 0 : maxRadius;
            for (int n5 = x - radius; n5 <= x + radius && clear; ++n5) {
                for (int n4 = z - radius; n4 <= z + radius && clear; ++n4) {
                    if (n7 >= 0 && n7 < 128) {
                        const int blockId = world->getBlockId(n5, n7, n4);
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
        int radius = random.nextInt(2);
        int radiusLimit = 1;
        int resetRadius = 0;
        for (int n3 = 0; n3 <= crownHeight; ++n3) {
            const int leafY = y + height - n3;
            for (int n = x - radius; n <= x + radius; ++n) {
                const int dx = n - x;
                for (int i = z - radius; i <= z + radius; ++i) {
                    const int dz = i - z;
                    if ((std::abs(dx) == radius && std::abs(dz) == radius && radius > 0)
                        || tree_feature::isOpaqueBlock(world->getBlockId(n, leafY, i))) {
                        continue;
                    }
                    world->setBlockWithoutNotifyingNeighbors(n, leafY, i, tree_feature::id(Block::LEAVES), 1);
                }
            }
            if (radius >= radiusLimit) {
                radius = resetRadius;
                resetRadius = 1;
                if (++radiusLimit > maxRadius) {
                    radiusLimit = maxRadius;
                }
                continue;
            }
            ++radius;
        }
        const int trunkShorten = random.nextInt(3);
        for (int n2 = 0; n2 < height - trunkShorten; ++n2) {
            const int blockId = world->getBlockId(x, y + n2, z);
            if (!tree_feature::canReplaceLeaves(blockId)) {
                continue;
            }
            world->setBlockWithoutNotifyingNeighbors(x, y + n2, z, tree_feature::id(Block::LOG), 1);
        }
        return true;
    }
};

} // namespace net::minecraft
