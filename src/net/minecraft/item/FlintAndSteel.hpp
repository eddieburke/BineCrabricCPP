#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemPlacement.hpp"

namespace net::minecraft::item {

class FlintAndSteel : public Item {
public:
    explicit FlintAndSteel(int rawId) : Item(rawId)
    {
        setMaxCount(1);
        setMaxDamage(64);
    }

    bool useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int side) override
    {
        if (world == nullptr || stack == nullptr) {
            return false;
        }
        detail::offsetPlacementPos(world, x, y, z, side);
        if (world->getBlockId(x, y, z) == 0 && Block::FIRE != nullptr) {
            world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5,
                "fire.ignite", 1.0f, random.nextFloat() * 0.4f + 0.8f);
            world->setBlock(x, y, z, Block::FIRE->id);
        }
        stack->applyDamage(1);
        return true;
    }
};

} // namespace net::minecraft::item
