#pragma once

#include "net/minecraft/item/FoodItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::item {

class MushroomStewItem : public FoodItem {
public:
    MushroomStewItem(int rawId, int healthRestored)
        : FoodItem(rawId, healthRestored, false)
    {
        setMaxCount(1);
    }

    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override
    {
        FoodItem::use(stack, world, user);
        return new ItemStack(Item::BOWL);
    }
};

} // namespace net::minecraft::item
