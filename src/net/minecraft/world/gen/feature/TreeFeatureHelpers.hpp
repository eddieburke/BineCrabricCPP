#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"

#include <cstdlib>

namespace net::minecraft::tree_feature {

inline int id(Block* block)
{
    return block != nullptr ? block->id : 0;
}

inline bool isOpaqueBlock(int blockId)
{
    if (blockId < 0 || blockId >= Block::BLOCK_COUNT) {
        return false;
    }
    return Block::BLOCKS_OPAQUE[static_cast<std::size_t>(blockId)];
}

inline bool canReplaceLeaves(int blockId)
{
    return blockId == 0 || blockId == id(Block::LEAVES);
}

inline bool isGrassOrDirt(int blockId)
{
    return blockId == id(Block::GRASS_BLOCK) || blockId == id(Block::DIRT);
}

inline bool generateRoundedTree(World* world, JavaRandom& random, int x, int y, int z, int height, int leafMeta, int logMeta)
{
    bool clear = true;
    if (world == nullptr || y < 1 || y + height + 1 > 128) {
        return false;
    }

    for (int n5 = y; n5 <= y + 1 + height; ++n5) {
        int radius = 1;
        if (n5 == y) {
            radius = 0;
        }
        if (n5 >= y + 1 + height - 2) {
            radius = 2;
        }
        for (int n3 = x - radius; n3 <= x + radius && clear; ++n3) {
            for (int n2 = z - radius; n2 <= z + radius && clear; ++n2) {
                if (n5 >= 0 && n5 < 128) {
                    const int blockId = world->getBlockId(n3, n5, n2);
                    if (canReplaceLeaves(blockId)) {
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
    if (!isGrassOrDirt(ground) || y >= 128 - height - 1) {
        return false;
    }

    world->setBlockWithoutNotifyingNeighbors(x, y - 1, z, id(Block::DIRT));
    for (int n4 = y - 3 + height; n4 <= y + height; ++n4) {
        const int n3 = n4 - (y + height);
        const int n2 = 1 - n3 / 2;
        for (int n = x - n2; n <= x + n2; ++n) {
            const int n7 = n - x;
            for (int i = z - n2; i <= z + n2; ++i) {
                const int n8 = i - z;
                if ((std::abs(n7) == n2 && std::abs(n8) == n2 && (random.nextInt(2) == 0 || n3 == 0))
                    || isOpaqueBlock(world->getBlockId(n, n4, i))) {
                    continue;
                }
                world->setBlockWithoutNotifyingNeighbors(n, n4, i, id(Block::LEAVES), leafMeta);
            }
        }
    }

    for (int n4 = 0; n4 < height; ++n4) {
        const int blockId = world->getBlockId(x, y + n4, z);
        if (!canReplaceLeaves(blockId)) {
            continue;
        }
        world->setBlockWithoutNotifyingNeighbors(x, y + n4, z, id(Block::LOG), logMeta);
    }
    return true;
}

} // namespace net::minecraft::tree_feature
