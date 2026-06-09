#include "net/minecraft/block/BlockRegistrar.hpp"
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
    int facing = FACE_WEST;
    if (blockView != nullptr) {
        const int northId = blockView->getBlockId(x, y, z - 1);
        const int southId = blockView->getBlockId(x, y, z + 1);
        const int westId = blockView->getBlockId(x - 1, y, z);
        const int eastId = blockView->getBlockId(x + 1, y, z);
        if (northId >= 0 && northId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)]
            && (southId < 0 || southId >= Block::BLOCK_COUNT || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)])) {
            facing = FACE_WEST;
        } else if (southId >= 0 && southId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(southId)]
                   && (northId < 0 || northId >= Block::BLOCK_COUNT || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(northId)])) {
            facing = FACE_EAST;
        } else if (westId >= 0 && westId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)]
                   && (eastId < 0 || eastId >= Block::BLOCK_COUNT || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)])) {
            facing = FACE_SOUTH;
        } else if (eastId >= 0 && eastId < Block::BLOCK_COUNT && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(eastId)]
                   && (westId < 0 || westId >= Block::BLOCK_COUNT || !Block::BLOCKS_OPAQUE[static_cast<std::size_t>(westId)])) {
            facing = FACE_NORTH;
        }
    }
    return Block::textureForSide(side, textureId, textureId - 1, textureId - 1, facing, textureId + 1);
}

int LockedChestBlock::getTexture(int side) const
{
    return Block::textureForSide(side, textureId, textureId - 1, textureId - 1, FACE_WEST, textureId + 1);
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
namespace {

void registerLockedChestBlock()
{
    Block::LOCKED_CHEST = (new LockedChestBlock(95))->setHardness(0.0f)->setLuminance(1.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("lockedchest")->setTickRandomly(true)->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerLockedChestBlock, 95);

} // namespace
} // namespace net::minecraft::block

