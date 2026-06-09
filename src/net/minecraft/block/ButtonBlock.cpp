#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/ButtonBlock.hpp"
#include "net/minecraft/block/Block.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

ButtonBlock::ButtonBlock(int blockId, int textureId)
    : Block(blockId, textureId, material::Material::PISTON_BREAKABLE)
{
    setTickRandomly(true);
}

bool ButtonBlock::canPlaceAt(World* world, int x, int y, int z, int side) const
{
    if (world == nullptr) {
        return false;
    }
    if (side == 2 && world->shouldSuffocate(x, y, z + 1)) {
        return true;
    }
    if (side == 3 && world->shouldSuffocate(x, y, z - 1)) {
        return true;
    }
    if (side == 4 && world->shouldSuffocate(x + 1, y, z)) {
        return true;
    }
    return side == 5 && world->shouldSuffocate(x - 1, y, z);
}

bool ButtonBlock::canPlaceAt(World* world, int x, int y, int z) const
{
    if (world == nullptr) {
        return false;
    }
    if (world->shouldSuffocate(x - 1, y, z)) {
        return true;
    }
    if (world->shouldSuffocate(x + 1, y, z)) {
        return true;
    }
    if (world->shouldSuffocate(x, y, z - 1)) {
        return true;
    }
    return world->shouldSuffocate(x, y, z + 1);
}

int ButtonBlock::getPlacementSide(World* world, int x, int y, int z) const
{
    if (world == nullptr) {
        return 1;
    }
    if (world->shouldSuffocate(x - 1, y, z)) {
        return 1;
    }
    if (world->shouldSuffocate(x + 1, y, z)) {
        return 2;
    }
    if (world->shouldSuffocate(x, y, z - 1)) {
        return 3;
    }
    if (world->shouldSuffocate(x, y, z + 1)) {
        return 4;
    }
    return 1;
}

void ButtonBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    const int meta = blockView != nullptr ? blockView->getBlockMeta(x, y, z) : 0;
    const int face = meta & 7;
    const bool pressed = (meta & 8) != 0;
    constexpr float yMin = 0.375f;
    constexpr float yMax = 0.625f;
    constexpr float halfWidth = 0.1875f;
    const float depth = pressed ? 0.0625f : 0.125f;

    if (face == 1) {
        setBoundingBox(0.0f, yMin, 0.5f - halfWidth, depth, yMax, 0.5f + halfWidth);
    } else if (face == 2) {
        setBoundingBox(1.0f - depth, yMin, 0.5f - halfWidth, 1.0f, yMax, 0.5f + halfWidth);
    } else if (face == 3) {
        setBoundingBox(0.5f - halfWidth, yMin, 0.0f, 0.5f + halfWidth, yMax, depth);
    } else if (face == 4) {
        setBoundingBox(0.5f - halfWidth, yMin, 1.0f - depth, 0.5f + halfWidth, yMax, 1.0f);
    }
}

void ButtonBlock::onPlaced(World* world, int x, int y, int z, int direction)
{
    if (world == nullptr) {
        return;
    }
    int meta = world->getBlockMeta(x, y, z);
    const int poweredBit = meta & 8;
    meta &= 7;
    if (direction == 2 && world->shouldSuffocate(x, y, z + 1)) {
        meta = 4;
    } else if (direction == 3 && world->shouldSuffocate(x, y, z - 1)) {
        meta = 3;
    } else if (direction == 4 && world->shouldSuffocate(x + 1, y, z)) {
        meta = 2;
    } else if (direction == 5 && world->shouldSuffocate(x - 1, y, z)) {
        meta = 1;
    } else {
        meta = getPlacementSide(world, x, y, z);
    }
    world->setBlockMeta(x, y, z, meta + poweredBit);
}

bool ButtonBlock::breakIfCannotPlaceAt(World* world, int x, int y, int z)
{
    if (world == nullptr) {
        return world != nullptr;
    }
    if (canPlaceAt(world, x, y, z)) {
        return true;
    }
    dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
    world->setBlock(x, y, z, 0);
    return false;
}

void ButtonBlock::neighborUpdate(World* world, int x, int y, int z, int /*id*/)
{
    if (world == nullptr || !breakIfCannotPlaceAt(world, x, y, z)) {
        return;
    }
    const int face = world->getBlockMeta(x, y, z) & 7;
    bool shouldBreak = false;
    if (!world->shouldSuffocate(x - 1, y, z) && face == 1) {
        shouldBreak = true;
    }
    if (!world->shouldSuffocate(x + 1, y, z) && face == 2) {
        shouldBreak = true;
    }
    if (!world->shouldSuffocate(x, y, z - 1) && face == 3) {
        shouldBreak = true;
    }
    if (!world->shouldSuffocate(x, y, z + 1) && face == 4) {
        shouldBreak = true;
    }
    if (shouldBreak) {
        dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, 0);
    }
}

void ButtonBlock::notifyAttachedNeighbors(World* world, int x, int y, int z, int meta)
{
    const int face = meta & 7;
    if (face == 1) {
        world->notifyNeighbors(x - 1, y, z, id);
    } else if (face == 2) {
        world->notifyNeighbors(x + 1, y, z, id);
    } else if (face == 3) {
        world->notifyNeighbors(x, y, z - 1, id);
    } else if (face == 4) {
        world->notifyNeighbors(x, y, z + 1, id);
    } else {
        world->notifyNeighbors(x, y - 1, z, id);
    }
}

void ButtonBlock::onBlockBreakStart(
    World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    onUse(world, x, y, z, player);
}

bool ButtonBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* /*player*/)
{
    if (world == nullptr) {
        return false;
    }
    const int meta = world->getBlockMeta(x, y, z);
    const int face = meta & 7;
    const int powered = 8 - (meta & 8);
    if (powered == 0) {
        return true;
    }
    world->setBlockMeta(x, y, z, face + powered);
    world->setBlocksDirty(x, y, z, x, y, z);
    world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5,
        "random.click", 0.3f, 0.6f);
    world->notifyNeighbors(x, y, z, id);
    notifyAttachedNeighbors(world, x, y, z, face);
    world->scheduleBlockUpdate(x, y, z, id, getTickRate());
    return true;
}

void ButtonBlock::onBreak(World* world, int x, int y, int z)
{
    if (world != nullptr && (world->getBlockMeta(x, y, z) & 8) > 0) {
        world->notifyNeighbors(x, y, z, id);
        notifyAttachedNeighbors(world, x, y, z, world->getBlockMeta(x, y, z));
    }
    Block::onBreak(world, x, y, z);
}

void ButtonBlock::onTick(
    World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (world == nullptr || world->isRemote()) {
        return;
    }
    const int meta = world->getBlockMeta(x, y, z);
    if ((meta & 8) == 0) {
        return;
    }
    world->setBlockMeta(x, y, z, meta & 7);
    world->notifyNeighbors(x, y, z, id);
    notifyAttachedNeighbors(world, x, y, z, meta);
    world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5,
        "random.click", 0.3f, 0.5f);
    world->setBlocksDirty(x, y, z, x, y, z);
}

bool ButtonBlock::canTransferPowerInDirection(
    World* world, int x, int y, int z, int direction) const
{
    if (world == nullptr || (world->getBlockMeta(x, y, z) & 8) == 0) {
        return false;
    }
    const int face = world->getBlockMeta(x, y, z) & 7;
    if (face == 5 && direction == 1) {
        return true;
    }
    if (face == 4 && direction == 2) {
        return true;
    }
    if (face == 3 && direction == 3) {
        return true;
    }
    if (face == 2 && direction == 4) {
        return true;
    }
    return face == 1 && direction == 5;
}

bool ButtonBlock::isEmittingRedstonePowerInDirection(
    const BlockView* blockView, int x, int y, int z, int /*direction*/) const
{
    return blockView != nullptr && (blockView->getBlockMeta(x, y, z) & 8) > 0;
}
namespace {

void registerButtonBlock()
{
    Block::BUTTON = (new ButtonBlock(77, Block::BLOCKS[1]->textureId))->setHardness(0.5f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("button")->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerButtonBlock, 77);

} // namespace
} // namespace net::minecraft::block

