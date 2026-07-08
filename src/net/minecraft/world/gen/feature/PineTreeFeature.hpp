#pragma once
#include <cstdlib>

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/gen/feature/Feature.hpp"
#include "net/minecraft/world/gen/feature/TreeFeatureHelpers.hpp"

namespace net::minecraft {
class World;

class PineTreeFeature : public Feature {
   public:
    bool generate(World* world, JavaRandom& random, int x, int y, int z) override {
        const int height = random.nextInt(5) + 7;
        const int trunkClearHeight = height - random.nextInt(2) - 3;
        const int crownHeight = height - trunkClearHeight;
        const int maxRadius = 1 + random.nextInt(crownHeight + 1);
        bool clear = true;
        if (world == nullptr || y < 1 || y + height + 1 > 128) {
            return false;
        }
        for (int checkY = y; checkY <= y + 1 + height && clear; ++checkY) {
            int radius = checkY - y < trunkClearHeight ? 0 : maxRadius;
            for (int blockX = x - radius; blockX <= x + radius && clear; ++blockX) {
                for (int blockZ = z - radius; blockZ <= z + radius && clear; ++blockZ) {
                    if (checkY >= 0 && checkY < 128) {
                        const int blockId = world->getBlockId(blockX, checkY, blockZ);
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
        for (int leafY = y + height; leafY >= y + trunkClearHeight; --leafY) {
            for (int leafX = x - radius; leafX <= x + radius; ++leafX) {
                const int dx = leafX - x;
                for (int leafZ = z - radius; leafZ <= z + radius; ++leafZ) {
                    const int dz = leafZ - z;
                    if ((std::abs(dx) == radius && std::abs(dz) == radius && radius > 0) ||
                        tree_feature::isOpaqueBlock(world->getBlockId(leafX, leafY, leafZ))) {
                        continue;
                    }
                    world->setBlockWithoutNotifyingNeighbors(leafX, leafY, leafZ, tree_feature::id(Block::LEAVES), 1);
                }
            }
            if (radius >= 1 && leafY == y + trunkClearHeight + 1) {
                --radius;
                continue;
            }
            if (radius < maxRadius) {
                ++radius;
            }
        }
        for (int logY = 0; logY < height - 1; ++logY) {
            const int blockId = world->getBlockId(x, y + logY, z);
            if (!tree_feature::canReplaceLeaves(blockId)) {
                continue;
            }
            world->setBlockWithoutNotifyingNeighbors(x, y + logY, z, tree_feature::id(Block::LOG), 1);
        }
        return true;
    }
};
}  // namespace net::minecraft
