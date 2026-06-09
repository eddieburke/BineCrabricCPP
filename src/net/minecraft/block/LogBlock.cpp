#include "net/minecraft/block/LogBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

LogBlock::LogBlock(int blockId) : Block(blockId, 20, material::Material::WOOD)
{
}

int LogBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const
{
    return Block::LOG != nullptr ? Block::LOG->id : id;
}

int LogBlock::getTexture(int side, int meta) const
{
    if (side == 1 || side == 0) {
        return 21;
    }
    if (meta == 1) {
        return 116;
    }
    if (meta == 2) {
        return 117;
    }
    return 20;
}

void LogBlock::onBreak(World* world, int x, int y, int z)
{
    if (world == nullptr || Block::LEAVES == nullptr) {
        return;
    }
    constexpr int radius = 4;
    const int loadedRadius = radius + 1;
    if (!world->isRegionLoaded(x - loadedRadius, y - loadedRadius, z - loadedRadius, x + loadedRadius, y + loadedRadius,
            z + loadedRadius)) {
        return;
    }
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dz = -radius; dz <= radius; ++dz) {
                const int leafX = x + dx;
                const int leafY = y + dy;
                const int leafZ = z + dz;
                if (world->getBlockId(leafX, leafY, leafZ) != Block::LEAVES->id) {
                    continue;
                }
                const int leafMeta = world->getBlockMeta(leafX, leafY, leafZ);
                if ((leafMeta & 8) != 0) {
                    continue;
                }
                world->setBlockMetaWithoutNotifyingNeighbors(leafX, leafY, leafZ, leafMeta | 8);
            }
        }
    }
}

} // namespace net::minecraft::block
