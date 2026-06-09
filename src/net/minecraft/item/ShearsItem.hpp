#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::item {

class ShearsItem : public Item {
public:
    explicit ShearsItem(int rawId) : Item(rawId)
    {
        setMaxCount(1);
        setMaxDamage(238);
    }

    bool postMine(ItemStack* stack, int blockId, int x, int y, int z, LivingEntity* miner) override
    {
        (void)x;
        (void)y;
        (void)z;
        (void)miner;
        if (stack != nullptr && ((Block::LEAVES != nullptr && blockId == Block::LEAVES->id)
                || (Block::COBWEB != nullptr && blockId == Block::COBWEB->id))) {
            stack->applyDamage(1);
        }
        return Item::postMine(stack, blockId, x, y, z, miner);
    }

    [[nodiscard]] bool isSuitableFor(Block* block) const override
    {
        return block != nullptr && Block::COBWEB != nullptr && block->id == Block::COBWEB->id;
    }

    [[nodiscard]] float getMiningSpeedMultiplier(ItemStack* stack, Block* block) const override
    {
        (void)stack;
        if (block == nullptr) {
            return Item::getMiningSpeedMultiplier(stack, block);
        }
        if ((Block::COBWEB != nullptr && block->id == Block::COBWEB->id) || (Block::LEAVES != nullptr && block->id == Block::LEAVES->id)) {
            return 15.0f;
        }
        if (Block::WOOL != nullptr && block->id == Block::WOOL->id) {
            return 5.0f;
        }
        return Item::getMiningSpeedMultiplier(stack, block);
    }
};

} // namespace net::minecraft::item
