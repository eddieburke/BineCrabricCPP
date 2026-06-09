#include "net/minecraft/client/SingleplayerInteractionManager.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

#include <cmath>

namespace net::minecraft::client {

SingleplayerInteractionManager::SingleplayerInteractionManager(Minecraft* minecraft)
    : InteractionManager(minecraft)
{
}

void SingleplayerInteractionManager::preparePlayer(PlayerEntity* player)
{
    if (player != nullptr) {
        player->yaw = -180.0f;
    }
}

bool SingleplayerInteractionManager::breakBlock(int x, int y, int z, int direction)
{
    if (minecraft == nullptr || minecraft->world == nullptr || minecraft->player == nullptr) {
        return false;
    }

    const int blockId = minecraft->world->getBlockId(x, y, z);
    const int blockMeta = minecraft->world->getBlockMeta(x, y, z);
    const bool removed = InteractionManager::breakBlock(x, y, z, direction);
    PlayerEntity* player = minecraft->player;
    ItemStack* handStack = player->inventory.getSelectedItem();
    const bool canHarvest = blockId > 0 && player->canHarvest(blockId);
    if (handStack != nullptr) {
        handStack->postMine(blockId, x, y, z, player);
        if (handStack->count == 0) {
            handStack->onRemoved(player);
            player->clearStackInHand();
        }
    }
    if (removed && canHarvest && blockId > 0 && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr) {
        Block::BLOCKS[static_cast<std::size_t>(blockId)]->afterBreak(minecraft->world, player, x, y, z, blockMeta);
    }
    return removed;
}

void SingleplayerInteractionManager::attackBlock(int x, int y, int z, int direction)
{
    if (minecraft == nullptr || minecraft->world == nullptr || minecraft->player == nullptr) {
        return;
    }

    minecraft->world->extinguishFire(minecraft->player, x, y, z, direction);
    const int blockId = minecraft->world->getBlockId(x, y, z);
    if (blockId > 0 && blockBreakingProgress == 0.0f) {
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block != nullptr) {
            block->onBlockBreakStart(minecraft->world, x, y, z, minecraft->player);
        }
    }
    if (blockId > 0 && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
        && Block::BLOCKS[static_cast<std::size_t>(blockId)]->getHardness(minecraft->player) >= 1.0f) {
        breakBlock(x, y, z, direction);
    }
}

void SingleplayerInteractionManager::cancelBlockBreaking()
{
    blockBreakingProgress = 0.0f;
    breakingDelayTicks = 0;
}

void SingleplayerInteractionManager::processBlockBreakingAction(int x, int y, int z, int side)
{
    if (breakingDelayTicks > 0) {
        --breakingDelayTicks;
        return;
    }

    if (x == breakingPosX && y == breakingPosY && z == breakingPosZ) {
        if (minecraft == nullptr || minecraft->world == nullptr || minecraft->player == nullptr) {
            return;
        }
        const int blockId = minecraft->world->getBlockId(x, y, z);
        if (blockId == 0) {
            return;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block == nullptr) {
            return;
        }
        blockBreakingProgress += block->getHardness(minecraft->player);
        if (std::fmod(breakingSoundDelayTicks, 4.0f) == 0.0f && block != nullptr) {
            minecraft->audio.playAt(
                block->soundGroup->getSound(),
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f,
                static_cast<float>(z) + 0.5f,
                (block->soundGroup->getVolume() + 1.0f) / 8.0f,
                block->soundGroup->getPitch() * 0.5f);
        }
        breakingSoundDelayTicks += 1.0f;
        if (blockBreakingProgress >= 1.0f) {
            breakBlock(x, y, z, side);
            blockBreakingProgress = 0.0f;
            lastBlockBreakingProgress = 0.0f;
            breakingSoundDelayTicks = 0.0f;
            breakingDelayTicks = 5;
        }
    } else {
        blockBreakingProgress = 0.0f;
        lastBlockBreakingProgress = 0.0f;
        breakingSoundDelayTicks = 0.0f;
        breakingPosX = x;
        breakingPosY = y;
        breakingPosZ = z;
    }
}

void SingleplayerInteractionManager::update(float partialTick)
{
    if (minecraft == nullptr) {
        return;
    }
    if (blockBreakingProgress <= 0.0f) {
        minecraft->inGameHud.progress = 0.0f;
        if (minecraft->worldRenderer != nullptr) {
            minecraft->worldRenderer->miningProgress = 0.0f;
        }
    } else {
        const float progress = lastBlockBreakingProgress + (blockBreakingProgress - lastBlockBreakingProgress) * partialTick;
        minecraft->inGameHud.progress = progress;
        if (minecraft->worldRenderer != nullptr) {
            minecraft->worldRenderer->miningProgress = progress;
        }
    }
}

float SingleplayerInteractionManager::getReachDistance()
{
    return 4.0f;
}

void SingleplayerInteractionManager::setWorld(World* world)
{
    InteractionManager::setWorld(world);
}

void SingleplayerInteractionManager::tick()
{
    lastBlockBreakingProgress = blockBreakingProgress;
    if (minecraft != nullptr) {
        minecraft->audio.tick();
    }
}

float SingleplayerInteractionManager::getBlockBreakingProgress(float partialTick) const
{
    return lastBlockBreakingProgress + (blockBreakingProgress - lastBlockBreakingProgress) * partialTick;
}

float SingleplayerInteractionManager::getLastBlockBreakingProgress() const
{
    return lastBlockBreakingProgress;
}

} // namespace net::minecraft::client
