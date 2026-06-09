#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

class MinecartItem : public Item {
public:
    MinecartItem(int rawId, int type) : Item(rawId), type_(type)
    {
        setMaxCount(1);
    }

    bool useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int /*side*/) override
    {
        if (world == nullptr || stack == nullptr || Block::RAIL == nullptr || world->getBlockId(x, y, z) != Block::RAIL->id) {
            return false;
        }
        if (!world->isRemote()) {
            auto* minecart = new entity::vehicle::MinecartEntity(world);
            minecart->type = type_;
            minecart->setPosition(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5);
            world->spawnEntity(minecart);
        }
        --stack->count;
        return true;
    }

private:
    int type_ = 0;
};

} // namespace net::minecraft::item
