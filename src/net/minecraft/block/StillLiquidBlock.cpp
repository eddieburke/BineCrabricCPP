#include "net/minecraft/block/StillLiquidBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

StillLiquidBlock::StillLiquidBlock(int id, Material& mat) : LiquidBlock(id, mat)
{
    setTickRandomly(false);
    if (&mat == &material::Material::LAVA) {
        setTickRandomly(true);
    }
}

void StillLiquidBlock::convertToFlowing(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    world->pauseTicking = true;
    world->setBlockWithoutNotifyingNeighbors(x, y, z, this->id - 1, meta);
    world->setBlocksDirty(x, y, z, x, y, z);
    world->scheduleBlockUpdate(x, y, z, this->id - 1, getTickRate());
    world->pauseTicking = false;
}

bool StillLiquidBlock::isFlammable(World* world, int x, int y, int z) const
{
    return world != nullptr && world->getMaterial(x, y, z).isBurnable();
}

void StillLiquidBlock::neighborUpdate(World* world, int x, int y, int z, int blockId)
{
    LiquidBlock::neighborUpdate(world, x, y, z, blockId);
    if (world != nullptr && world->getBlockId(x, y, z) == id) {
        convertToFlowing(world, x, y, z);
    }
}

void StillLiquidBlock::onTick(World* world, int x, int y, int z, JavaRandom& random)
{
    if (world == nullptr || &material != &material::Material::LAVA) {
        return;
    }
    const int attempts = random.nextInt(3);
    for (int i = 0; i < attempts; ++i) {
        x += random.nextInt(3) - 1;
        ++y;
        z += random.nextInt(3) - 1;
        const int blockId = world->getBlockId(x, y, z);
        if (blockId == 0) {
            if (!isFlammable(world, x - 1, y, z) && !isFlammable(world, x + 1, y, z)
                && !isFlammable(world, x, y, z - 1) && !isFlammable(world, x, y, z + 1)
                && !isFlammable(world, x, y - 1, z) && !isFlammable(world, x, y + 1, z)) {
                continue;
            }
            if (Block::FIRE != nullptr) {
                world->setBlock(x, y, z, Block::FIRE->id);
            }
            return;
        }
        if (blockId >= Block::BLOCK_COUNT || Block::BLOCKS[static_cast<std::size_t>(blockId)] == nullptr
            || !Block::BLOCKS[static_cast<std::size_t>(blockId)]->material.blocksMovement()) {
            continue;
        }
        return;
    }
}

} // namespace net::minecraft::block
