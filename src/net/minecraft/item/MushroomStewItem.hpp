#pragma once

#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft

namespace net::minecraft::item {

class MushroomStewItem : public FoodItem {
public:
    MushroomStewItem(int rawId, int healthRestored);
    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override;
};

} // namespace net::minecraft::item
