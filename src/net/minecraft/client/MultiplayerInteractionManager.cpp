#include "net/minecraft/client/MultiplayerInteractionManager.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/network/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/packet/InventoryPackets.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <cmath>
#include <utility>

namespace net::minecraft::client {

MultiplayerInteractionManager::MultiplayerInteractionManager(Minecraft* minecraft, network::ClientNetworkHandler* networkHandler)
    : InteractionManager(minecraft),
      networkHandler(networkHandler)
{
}

void MultiplayerInteractionManager::preparePlayer(PlayerEntity* player)
{
    if (player != nullptr) {
        player->yaw = -180.0f;
    }
}

bool MultiplayerInteractionManager::breakBlock(int x, int y, int z, int direction)
{
    if (minecraft == nullptr || minecraft->world == nullptr || minecraft->player == nullptr) {
        return false;
    }

    const int blockId = minecraft->world->getBlockId(x, y, z);
    const bool removed = InteractionManager::breakBlock(x, y, z, direction);
    ItemStack* handStack = minecraft->player->inventory.getSelectedItem();
    if (handStack != nullptr) {
        handStack->postMine(blockId, x, y, z, minecraft->player);
        if (handStack->count == 0) {
            handStack->onRemoved(minecraft->player);
            minecraft->player->clearStackInHand();
        }
    }
    return removed;
}

void MultiplayerInteractionManager::attackBlock(int x, int y, int z, int direction)
{
    if (minecraft == nullptr || minecraft->world == nullptr || minecraft->player == nullptr) {
        return;
    }

    World* world = minecraft->world;
    PlayerEntity* player = minecraft->player;

    if (!breakingBlock || x != breakingPosX || y != breakingPosY || z != breakingPosZ) {
        if (networkHandler != nullptr) {
            PlayerActionC2SPacket packet;
            packet.action = 0;
            packet.x = x;
            packet.y = y;
            packet.z = z;
            packet.direction = direction;
            networkHandler->sendPacket(packet);
        }
        const int blockId = world->getBlockId(x, y, z);
        if (blockId > 0 && blockBreakingProgress == 0.0f && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr) {
            Block::BLOCKS[static_cast<std::size_t>(blockId)]->onBlockBreakStart(world, x, y, z, player);
        }
        if (blockId > 0 && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
            && Block::BLOCKS[static_cast<std::size_t>(blockId)]->getHardness(player) >= 1.0f) {
            breakBlock(x, y, z, direction);
        } else {
            breakingBlock = true;
            breakingPosX = x;
            breakingPosY = y;
            breakingPosZ = z;
            blockBreakingProgress = 0.0f;
            lastBlockBreakingProgress = 0.0f;
            breakingSoundDelayTicks = 0.0f;
        }
    }
}

void MultiplayerInteractionManager::cancelBlockBreaking()
{
    blockBreakingProgress = 0.0f;
    breakingBlock = false;
}

void MultiplayerInteractionManager::processBlockBreakingAction(int x, int y, int z, int side)
{
    if (!breakingBlock) {
        return;
    }

    updateSelectedSlot();
    if (breakingDelayTicks > 0) {
        --breakingDelayTicks;
        return;
    }

    if (x == breakingPosX && y == breakingPosY && z == breakingPosZ) {
        if (minecraft == nullptr || minecraft->world == nullptr || minecraft->player == nullptr) {
            return;
        }
        World* world = minecraft->world;
        PlayerEntity* player = minecraft->player;
        const int blockId = world->getBlockId(x, y, z);
        if (blockId == 0) {
            breakingBlock = false;
            return;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block == nullptr) {
            return;
        }
        blockBreakingProgress += block->getHardness(player);
        if (std::fmod(breakingSoundDelayTicks, 4.0f) == 0.0f && block != nullptr) {
            minecraft->audio.playAt(block->soundGroup->getSound(), static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, static_cast<float>(z) + 0.5f, (block->soundGroup->getVolume() + 1.0f) / 8.0f, block->soundGroup->getPitch() * 0.5f);
        }
        breakingSoundDelayTicks += 1.0f;
        if (blockBreakingProgress >= 1.0f) {
            breakingBlock = false;
            if (networkHandler != nullptr) {
                PlayerActionC2SPacket packet;
                packet.action = 2;
                packet.x = x;
                packet.y = y;
                packet.z = z;
                packet.direction = side;
                networkHandler->sendPacket(packet);
            }
            breakBlock(x, y, z, side);
            blockBreakingProgress = 0.0f;
            lastBlockBreakingProgress = 0.0f;
            breakingSoundDelayTicks = 0.0f;
            breakingDelayTicks = 5;
        }
    } else {
        attackBlock(x, y, z, side);
    }
}

void MultiplayerInteractionManager::update(float partialTick)
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

float MultiplayerInteractionManager::getReachDistance()
{
    return 4.0f;
}

void MultiplayerInteractionManager::setWorld(World* world)
{
    InteractionManager::setWorld(world);
}

void MultiplayerInteractionManager::tick()
{
    updateSelectedSlot();
    lastBlockBreakingProgress = blockBreakingProgress;
    if (minecraft != nullptr) {
        minecraft->audio.tick();
    }
}

bool MultiplayerInteractionManager::interactBlock(PlayerEntity* player, World* world, ItemStack* item, int x, int y, int z, int side)
{
    updateSelectedSlot();
    if (networkHandler != nullptr && player != nullptr) {
        PlayerInteractBlockC2SPacket packet;
        packet.x = x;
        packet.y = y;
        packet.z = z;
        packet.side = side;
        if (ItemStack* selected = player->inventory.getSelectedItem()) {
            packet.stack = *selected;
        }
        networkHandler->sendPacket(packet);
    }
    return InteractionManager::interactBlock(player, world, item, x, y, z, side);
}

bool MultiplayerInteractionManager::interactItem(PlayerEntity* player, World* world, ItemStack* item)
{
    updateSelectedSlot();
    if (networkHandler != nullptr && player != nullptr) {
        PlayerInteractBlockC2SPacket packet;
        packet.x = -1;
        packet.y = -1;
        packet.z = -1;
        packet.side = 255;
        if (ItemStack* selected = player->inventory.getSelectedItem()) {
            packet.stack = *selected;
        }
        networkHandler->sendPacket(packet);
    }
    return InteractionManager::interactItem(player, world, item);
}

PlayerEntity* MultiplayerInteractionManager::createPlayer(World* world)
{
    return new network::MultiplayerClientPlayerEntity(minecraft, world, minecraft->session, networkHandler);
}

void MultiplayerInteractionManager::attackEntity(PlayerEntity* player, Entity* target)
{
    updateSelectedSlot();
    if (networkHandler != nullptr && player != nullptr && target != nullptr) {
        PlayerInteractEntityC2SPacket packet;
        packet.playerId = player->id;
        packet.entityId = target->id;
        packet.isLeftClick = 1;
        networkHandler->sendPacket(packet);
    }
    if (player != nullptr && target != nullptr) {
        player->attack(target);
    }
}

void MultiplayerInteractionManager::interactEntity(PlayerEntity* player, Entity* entity)
{
    updateSelectedSlot();
    if (networkHandler != nullptr && player != nullptr && entity != nullptr) {
        PlayerInteractEntityC2SPacket packet;
        packet.playerId = player->id;
        packet.entityId = entity->id;
        packet.isLeftClick = 0;
        networkHandler->sendPacket(packet);
    }
    if (player != nullptr && entity != nullptr) {
        player->interact(entity);
    }
}

ItemStack* MultiplayerInteractionManager::clickSlot(int syncId, int slotId, int button, bool shift, PlayerEntity* player)
{
    std::uint16_t revision = 0;
    if (player != nullptr && player->currentScreenHandler != nullptr) {
        revision = player->currentScreenHandler->nextRevision();
    }
    ItemStack* itemStack = InteractionManager::clickSlot(syncId, slotId, button, shift, player);
    if (networkHandler != nullptr) {
        ClickSlotC2SPacket packet;
        packet.syncId = syncId;
        packet.slot = slotId;
        packet.button = button;
        packet.holdingShift = shift;
        packet.actionType = static_cast<std::int16_t>(revision);
        if (itemStack != nullptr) {
            packet.stack = *itemStack;
        }
        networkHandler->sendPacket(packet);
    }
    return itemStack;
}

void MultiplayerInteractionManager::onScreenRemoved(int syncId, PlayerEntity* /*player*/)
{
    if (syncId == -9999) {
        return;
    }
}

void MultiplayerInteractionManager::updateSelectedSlot()
{
    if (minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }

    const int selected = minecraft->player->inventory.selectedSlot;
    if (selected != selectedSlot) {
        selectedSlot = selected;
        if (networkHandler != nullptr) {
            UpdateSelectedSlotC2SPacket packet;
            packet.selectedSlot = selectedSlot;
            networkHandler->sendPacket(packet);
        }
    }
}

} // namespace net::minecraft::client
