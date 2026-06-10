#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/LeverBlock.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

LeverBlock::LeverBlock(int blockId, int textureId) : Block(blockId, textureId, material::Material::PISTON_BREAKABLE)
{
}

bool LeverBlock::canPlaceAt(World* world, int x, int y, int z, int side) const
{
    if (world == nullptr) {
        return false;
    }
    if (side == 1 && world->shouldSuffocate(x, y - 1, z)) {
        return true;
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

bool LeverBlock::canPlaceAt(World* world, int x, int y, int z) const
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
    if (world->shouldSuffocate(x, y, z + 1)) {
        return true;
    }
    return world->shouldSuffocate(x, y - 1, z);
}

void LeverBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    setBoundingBox(getRenderBounds(blockView, x, y, z));
}

net::minecraft::Box LeverBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    const int face = blockView != nullptr ? (blockView->getBlockMeta(x, y, z) & 7) : 0;
    constexpr float halfThickness = 0.1875f;
    if (face == 1) {
        return {0.0f, 0.2f, 0.5f - halfThickness, halfThickness * 2.0f, 0.8f, 0.5f + halfThickness};
    }
    if (face == 2) {
        return {1.0f - halfThickness * 2.0f, 0.2f, 0.5f - halfThickness, 1.0f, 0.8f, 0.5f + halfThickness};
    }
    if (face == 3) {
        return {0.5f - halfThickness, 0.2f, 0.0f, 0.5f + halfThickness, 0.8f, halfThickness * 2.0f};
    }
    if (face == 4) {
        return {0.5f - halfThickness, 0.2f, 1.0f - halfThickness * 2.0f, 0.5f + halfThickness, 0.8f, 1.0f};
    }
    constexpr float halfWidth = 0.25f;
    return {0.5f - halfWidth, 0.0f, 0.5f - halfWidth, 0.5f + halfWidth, 0.6f, 0.5f + halfWidth};
}

void LeverBlock::onPlaced(World* world, int x, int y, int z, int direction)
{
    if (world == nullptr) {
        return;
    }
    int meta = world->getBlockMeta(x, y, z);
    const int poweredBit = meta & 8;
    meta &= 7;
    meta = -1;
    if (direction == 1 && world->shouldSuffocate(x, y - 1, z)) {
        meta = 5 + world->random().nextInt(2);
    }
    if (direction == 2 && world->shouldSuffocate(x, y, z + 1)) {
        meta = 4;
    }
    if (direction == 3 && world->shouldSuffocate(x, y, z - 1)) {
        meta = 3;
    }
    if (direction == 4 && world->shouldSuffocate(x + 1, y, z)) {
        meta = 2;
    }
    if (direction == 5 && world->shouldSuffocate(x - 1, y, z)) {
        meta = 1;
    }
    if (meta == -1) {
        dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, 0);
        return;
    }
    world->setBlockMeta(x, y, z, meta + poweredBit);
}

bool LeverBlock::breakIfCannotPlaceAt(World* world, int x, int y, int z)
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

void LeverBlock::neighborUpdate(World* world, int x, int y, int z, int /*id*/)
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
    if (!world->shouldSuffocate(x, y - 1, z) && (face == 5 || face == 6)) {
        shouldBreak = true;
    }
    if (shouldBreak) {
        dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
        world->setBlock(x, y, z, 0);
    }
}

void LeverBlock::notifyAttachedNeighbors(World* world, int x, int y, int z, int meta)
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

void LeverBlock::onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player)
{
    onUse(world, x, y, z, player);
}

bool LeverBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* /*player*/)
{
    if (world == nullptr) {
        return false;
    }
    if (world->isRemote()) {
        return true;
    }
    const int meta = world->getBlockMeta(x, y, z);
    const int face = meta & 7;
    const int powered = 8 - (meta & 8);
    world->setBlockMeta(x, y, z, face + powered);
    world->setBlocksDirty(x, y, z, x, y, z);
    world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5, "random.click", 0.3f,
        powered > 0 ? 0.6f : 0.5f);
    world->notifyNeighbors(x, y, z, id);
    notifyAttachedNeighbors(world, x, y, z, face);
    return true;
}

void LeverBlock::onBreak(World* world, int x, int y, int z)
{
    if (world != nullptr && (world->getBlockMeta(x, y, z) & 8) > 0) {
        world->notifyNeighbors(x, y, z, id);
        notifyAttachedNeighbors(world, x, y, z, world->getBlockMeta(x, y, z));
    }
    Block::onBreak(world, x, y, z);
}

bool LeverBlock::canTransferPowerInDirection(
    World* world, int x, int y, int z, int direction) const
{
    if (world == nullptr || (world->getBlockMeta(x, y, z) & 8) == 0) {
        return false;
    }
    const int face = world->getBlockMeta(x, y, z) & 7;
    if (face == 6 && direction == 1) {
        return true;
    }
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

bool LeverBlock::isEmittingRedstonePowerInDirection(
    const BlockView* blockView, int x, int y, int z, int /*direction*/) const
{
    return blockView != nullptr && (blockView->getBlockMeta(x, y, z) & 8) > 0;
}
namespace {

void registerLeverBlock()
{
    Block::LEVER = (new LeverBlock(69, 96))->setHardness(0.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("lever")->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerLeverBlock, 69);

} // namespace
} // namespace net::minecraft::block

