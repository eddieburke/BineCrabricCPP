#include "net/minecraft/block/LockedChestBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

LockedChestBlock::LockedChestBlock(int blockId) : Block(blockId, material::Material::WOOD)
{
    textureId = 26;
}

int LockedChestBlock::getTextureId(const BlockView* blockView, int x, int y, int z, int side) const
{
    if (side == 1 || side == 0) {
        return textureId - 1;
    }
    if (blockView == nullptr) {
        return textureId;
    }
    const int northId = blockView->getBlockId(x, y, z - 1);
    const int southId = blockView->getBlockId(x, y, z + 1);
    const int westId = blockView->getBlockId(x - 1, y, z);
    const int eastId = blockView->getBlockId(x + 1, y, z);
    int facing = 3;
    if (northId >= 0 && northId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)]
        && (southId < 0 || southId >= Block::BLOCK_COUNT || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)])) {
        facing = 3;
    } else if (southId >= 0 && southId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)]
               && (northId < 0 || northId >= Block::BLOCK_COUNT || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)])) {
        facing = 2;
    } else if (westId >= 0 && westId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)]
               && (eastId < 0 || eastId >= Block::BLOCK_COUNT || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)])) {
        facing = 5;
    } else if (eastId >= 0 && eastId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)]
               && (westId < 0 || westId >= Block::BLOCK_COUNT || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)])) {
        facing = 4;
    }
    return side == facing ? textureId + 1 : textureId;
}

int LockedChestBlock::getTexture(int side) const
{
    if (side == 1 || side == 0) {
        return textureId - 1;
    }
    if (side == 3) {
        return textureId + 1;
    }
    return textureId;
}

bool LockedChestBlock::canPlaceAt(World* /*world*/, int /*x*/, int /*y*/, int /*z*/) const
{
    return true;
}

void LockedChestBlock::onTick(World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (world != nullptr) {
        world->setBlock(x, y, z, 0);
    }
}

} // namespace net::minecraft::block
