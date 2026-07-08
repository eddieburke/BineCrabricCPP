#pragma once
#include <cstdlib>

#include "net/minecraft/block/Block.hpp"

namespace net::minecraft::tree_feature {
inline int id(Block* block) {
    return block != nullptr ? block->id : 0;
}

inline bool isOpaqueBlock(int blockId) {
    if (blockId < 0 || blockId >= Block::BLOCK_COUNT) {
        return false;
    }
    return Block::BLOCKS_OPAQUE[static_cast<std::size_t>(blockId)];
}

inline bool canReplaceLeaves(int blockId) {
    return blockId == 0 || blockId == id(Block::LEAVES);
}

inline bool isGrassOrDirt(int blockId) {
    return blockId == id(Block::GRASS_BLOCK) || blockId == id(Block::DIRT);
}

inline bool generateRoundedTree(
    World* world, JavaRandom& random, int x, int y, int z, int height, int leafMeta, int logMeta) {
    bool clear = true;
    if (world == nullptr || y < 1 || y + height + 1 > 128) {
        return false;
    }
    for (int checkY = y; checkY <= y + 1 + height; ++checkY) {
        int radius = 1;
        if (checkY == y) {
            radius = 0;
        }
        if (checkY >= y + 1 + height - 2) {
            radius = 2;
        }
        for (int blockX = x - radius; blockX <= x + radius && clear; ++blockX) {
            for (int blockZ = z - radius; blockZ <= z + radius && clear; ++blockZ) {
                if (checkY >= 0 && checkY < 128) {
                    const int blockId = world->getBlockId(blockX, checkY, blockZ);
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
    for (int leafY = y - 3 + height; leafY <= y + height; ++leafY) {
        const int layerOffset = leafY - (y + height);
        const int leafRadius = 1 - layerOffset / 2;
        for (int leafX = x - leafRadius; leafX <= x + leafRadius; ++leafX) {
            const int dx = leafX - x;
            for (int leafZ = z - leafRadius; leafZ <= z + leafRadius; ++leafZ) {
                const int dz = leafZ - z;
                if ((std::abs(dx) == leafRadius && std::abs(dz) == leafRadius &&
                     (random.nextInt(2) == 0 || layerOffset == 0)) ||
                    isOpaqueBlock(world->getBlockId(leafX, leafY, leafZ))) {
                    continue;
                }
                world->setBlockWithoutNotifyingNeighbors(leafX, leafY, leafZ, id(Block::LEAVES), leafMeta);
            }
        }
    }
    for (int logY = 0; logY < height; ++logY) {
        const int blockId = world->getBlockId(x, y + logY, z);
        if (!canReplaceLeaves(blockId)) {
            continue;
        }
        world->setBlockWithoutNotifyingNeighbors(x, y + logY, z, id(Block::LOG), logMeta);
    }
    return true;
}
}  // namespace net::minecraft::tree_feature
