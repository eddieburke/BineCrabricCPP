#include "net/minecraft/server/network/ServerPlayerInteractionManager.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/GameHooks.hpp"
#include "net/minecraft/network/packet/BlockPackets.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::server::network {
ServerPlayerInteractionManager::ServerPlayerInteractionManager(World* worldIn) : world(worldIn) {
}

void ServerPlayerInteractionManager::update() {
    ++tickCounter_;
    if (!mining_) {
        return;
    }
    const int elapsed = tickCounter_ - startMiningTime_;
    const int blockId = world != nullptr ? world->getBlockId(miningX_, miningY_, miningZ_) : 0;
    if (blockId == 0) {
        mining_ = false;
        return;
    }
    Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
    if (block == nullptr) {
        mining_ = false;
        return;
    }
    const float progress = block->getHardness(player) * static_cast<float>(elapsed + 1);
    if (progress >= 1.0f) {
        mining_ = false;
        tryBreakBlock(miningX_, miningY_, miningZ_);
    }
}

void ServerPlayerInteractionManager::onBlockBreakingAction(int x, int y, int z, int direction) {
    if (world == nullptr) {
        return;
    }
    world->extinguishFire(nullptr, x, y, z, direction);
    failedMiningStartTime_ = tickCounter_;
    const int blockId = world->getBlockId(x, y, z);
    if (blockId > 0) {
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block != nullptr) {
            block->onBlockBreakStart(world, x, y, z, player);
        }
    }
    if (blockId > 0 && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr &&
        Block::BLOCKS[static_cast<std::size_t>(blockId)]->getHardness(player) >= 1.0f) {
        tryBreakBlock(x, y, z);
    } else {
        failedMiningX_ = x;
        failedMiningY_ = y;
        failedMiningZ_ = z;
    }
}

void ServerPlayerInteractionManager::continueMining(int x, int y, int z) {
    if (world == nullptr) {
        return;
    }
    if (x == failedMiningX_ && y == failedMiningY_ && z == failedMiningZ_) {
        const int elapsed = tickCounter_ - failedMiningStartTime_;
        const int blockId = world->getBlockId(x, y, z);
        if (blockId != 0) {
            Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
            if (block != nullptr) {
                const float progress = block->getHardness(player) * static_cast<float>(elapsed + 1);
                if (progress >= 0.7f) {
                    tryBreakBlock(x, y, z);
                } else if (!mining_) {
                    mining_ = true;
                    miningX_ = x;
                    miningY_ = y;
                    miningZ_ = z;
                    startMiningTime_ = failedMiningStartTime_;
                }
            }
        }
    }
    blockBreakProgress_ = 0.0f;
}

bool ServerPlayerInteractionManager::finishMining(int x, int y, int z) {
    if (world == nullptr) {
        return false;
    }
    const int blockId = world->getBlockId(x, y, z);
    Block* block = blockId > 0 ? Block::BLOCKS[static_cast<std::size_t>(blockId)] : nullptr;
    const int meta = world->getBlockMeta(x, y, z);
    const bool removed = world->setBlock(x, y, z, 0);
    if (block != nullptr && removed) {
        block->onMetadataChange(world, x, y, z, meta);
    }
    return removed;
}

bool ServerPlayerInteractionManager::tryBreakBlock(int x, int y, int z) {
    if (world == nullptr || player == nullptr) {
        return false;
    }
    const int blockId = world->getBlockId(x, y, z);
    const int meta = world->getBlockMeta(x, y, z);
    world->worldEvent(player, 2001, x, y, z, blockId + meta * 256);
    const bool removed = finishMining(x, y, z);
    ItemStack handStack = player->getHand();
    if (!handStack.empty() && handStack.getItem() != nullptr) {
        handStack.getItem()->postMine(&handStack, blockId, x, y, z, player);
        if (handStack.count == 0) {
            handStack.onRemoved(player);
            player->clearStackInHand();
        }
    }
    Block* brokenBlock = blockId > 0 ? Block::BLOCKS[static_cast<std::size_t>(blockId)] : nullptr;
    if (removed && brokenBlock != nullptr && player->canHarvest(blockId)) {
        brokenBlock->afterBreak(world, player, x, y, z, meta);
        if (auto* serverPlayer = dynamic_cast<::net::minecraft::entity::player::ServerPlayerEntity*>(player);
            serverPlayer != nullptr && serverPlayer->networkHandler != nullptr) {
            BlockUpdateS2CPacket blockPacket;
            blockPacket.x = x;
            blockPacket.y = y;
            blockPacket.z = z;
            blockPacket.blockRawId = world->getBlockId(x, y, z);
            blockPacket.blockMetadata = world->getBlockMeta(x, y, z);
            serverPlayer->networkHandler->sendPacket(blockPacket);
        }
    }
    return removed;
}

bool ServerPlayerInteractionManager::interactItem(::net::minecraft::entity::player::PlayerEntity* playerIn,
                                                  World* worldIn,
                                                  ItemStack* stack) {
    if (playerIn == nullptr || worldIn == nullptr || stack == nullptr) {
        return false;
    }
    const int previousCount = stack->count;
    ItemStack* used = stack->getItem() != nullptr ? stack->getItem()->use(stack, worldIn, playerIn) : nullptr;
    if (used != stack || (used != nullptr && used->count != previousCount)) {
        const std::size_t selectedSlot = static_cast<std::size_t>(playerIn->inventory.selectedSlot);
        playerIn->inventory.main[selectedSlot] = used != nullptr ? *used : ItemStack{};
        if (playerIn->inventory.main[selectedSlot].empty()) {
            playerIn->inventory.main[selectedSlot] = ItemStack{};
        }
        return true;
    }
    return false;
}

bool ServerPlayerInteractionManager::interactBlock(::net::minecraft::entity::player::PlayerEntity* playerIn,
                                                   World* worldIn,
                                                   ItemStack* stack,
                                                   int x,
                                                   int y,
                                                   int z,
                                                   int side) {
    if (worldIn == nullptr || playerIn == nullptr) {
        return false;
    }
    mod::BlockInteractEvent event{playerIn, worldIn, stack, x, y, z, side, true, false, false};
    mod::hooks().publish(event);
    if (event.canceled) {
        return event.handled;
    }
    stack = event.stack;
    x = event.x;
    y = event.y;
    z = event.z;
    side = event.side;
    if (event.handled) {
        return true;
    }
    const int blockId = worldIn->getBlockId(x, y, z);
    if (blockId > 0) {
        Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if (block != nullptr && block->onUse(worldIn, x, y, z, playerIn)) {
            return true;
        }
    }
    if (stack == nullptr) {
        return false;
    }
    return stack->getItem() != nullptr ? stack->getItem()->useOnBlock(stack, playerIn, worldIn, x, y, z, side) : false;
}
}  // namespace net::minecraft::server::network
