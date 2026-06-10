#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/PressurePlateBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/block/PressurePlateActivationRule.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::block {

PressurePlateBlock::PressurePlateBlock(
    int blockId, int textureId, PressurePlateActivationRule activationRuleIn, Material& material)
    : Block(blockId, textureId, material)
{
    activationRule = activationRuleIn;
    setTickRandomly(true);
    const float inset = 0.0625f;
    setBoundingBox(inset, 0.0f, inset, 1.0f - inset, 0.03125f, 1.0f - inset);
}

bool PressurePlateBlock::canPlaceAt(World* world, int x, int y, int z) const
{
    return world != nullptr && world->shouldSuffocate(x, y - 1, z);
}

void PressurePlateBlock::neighborUpdate(World* world, int x, int y, int z, int /*neighborId*/)
{
    if (world == nullptr) {
        return;
    }
    if (world->shouldSuffocate(x, y - 1, z)) {
        return;
    }
    dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
    world->setBlock(x, y, z, 0);
}

void PressurePlateBlock::updatePlateState(World* world, int x, int y, int z)
{
    const bool pressed = world->getBlockMeta(x, y, z) == 1;
    bool shouldPress = false;
    constexpr float inset = 0.125f;
    const net::minecraft::Box box(
        static_cast<double>(x) + inset,
        static_cast<double>(y),
        static_cast<double>(z) + inset,
        static_cast<double>(x + 1) - inset,
        static_cast<double>(y) + 0.25,
        static_cast<double>(z + 1) - inset);

    if (activationRule == PressurePlateActivationRule::EVERYTHING) {
        shouldPress = !world->getEntities(nullptr, box).empty();
    } else if (activationRule == PressurePlateActivationRule::MOBS) {
        for (Entity* entity : world->getEntities(nullptr, box)) {
            if (dynamic_cast<LivingEntity*>(entity) != nullptr) {
                shouldPress = true;
                break;
            }
        }
    } else if (activationRule == PressurePlateActivationRule::PLAYERS) {
        for (Entity* entity : world->getEntities(nullptr, box)) {
            if (dynamic_cast<PlayerEntity*>(entity) != nullptr) {
                shouldPress = true;
                break;
            }
        }
    }

    if (shouldPress && !pressed) {
        world->setBlockMeta(x, y, z, 1);
        world->notifyNeighbors(x, y, z, id);
        world->notifyNeighbors(x, y - 1, z, id);
        world->setBlocksDirty(x, y, z, x, y, z);
        world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.1, static_cast<double>(z) + 0.5,
            "random.click", 0.3f, 0.6f);
    } else if (!shouldPress && pressed) {
        world->setBlockMeta(x, y, z, 0);
        world->notifyNeighbors(x, y, z, id);
        world->notifyNeighbors(x, y - 1, z, id);
        world->setBlocksDirty(x, y, z, x, y, z);
        world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.1, static_cast<double>(z) + 0.5,
            "random.click", 0.3f, 0.5f);
    }
    if (shouldPress) {
        world->scheduleBlockUpdate(x, y, z, id, getTickRate());
    }
}

void PressurePlateBlock::onTick(
    World* world, int x, int y, int z, JavaRandom& /*random*/)
{
    if (world == nullptr || world->isRemote() || world->getBlockMeta(x, y, z) == 0) {
        return;
    }
    updatePlateState(world, x, y, z);
}

void PressurePlateBlock::onEntityCollision(
    World* world, int x, int y, int z, net::minecraft::Entity* /*entity*/)
{
    if (world == nullptr || world->isRemote() || world->getBlockMeta(x, y, z) == 1) {
        return;
    }
    updatePlateState(world, x, y, z);
}

void PressurePlateBlock::onBreak(World* world, int x, int y, int z)
{
    if (world != nullptr && world->getBlockMeta(x, y, z) > 0) {
        world->notifyNeighbors(x, y, z, id);
        world->notifyNeighbors(x, y - 1, z, id);
    }
    Block::onBreak(world, x, y, z);
}

void PressurePlateBlock::updateBoundingBox(const BlockView* blockView, int x, int y, int z)
{
    setBoundingBox(getRenderBounds(blockView, x, y, z));
}

net::minecraft::Box PressurePlateBlock::getRenderBounds(const BlockView* blockView, int x, int y, int z) const
{
    const bool pressed = blockView != nullptr && blockView->getBlockMeta(x, y, z) == 1;
    const float inset = 0.0625f;
    const float height = pressed ? 0.03125f : 0.0625f;
    return {inset, 0.0f, inset, 1.0f - inset, height, 1.0f - inset};
}

bool PressurePlateBlock::isEmittingRedstonePowerInDirection(
    const BlockView* blockView, int x, int y, int z, int /*direction*/) const
{
    return blockView != nullptr && blockView->getBlockMeta(x, y, z) > 0;
}

bool PressurePlateBlock::canTransferPowerInDirection(
    World* world, int x, int y, int z, int direction) const
{
    return world != nullptr && world->getBlockMeta(x, y, z) != 0 && direction == 1;
}
namespace {

void registerPressurePlateBlocks()
{
    namespace mat = material;
    Block::STONE_PRESSURE_PLATE = (new PressurePlateBlock(70, Block::BLOCKS[1]->textureId, PressurePlateActivationRule::MOBS, mat::Material::STONE))->setHardness(0.5f)->setSoundGroup(&vanillaStoneSound())->setTranslationKey("pressurePlate")->ignoreMetaUpdates();
    Block::WOODEN_PRESSURE_PLATE = (new PressurePlateBlock(72, Block::BLOCKS[5]->textureId, PressurePlateActivationRule::EVERYTHING, mat::Material::WOOD))->setHardness(0.5f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("pressurePlate")->ignoreMetaUpdates();
}

MINECRAFT_REGISTER_BLOCK(registerPressurePlateBlocks, 72);

} // namespace
} // namespace net::minecraft::block

