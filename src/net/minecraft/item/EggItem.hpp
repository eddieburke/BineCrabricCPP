#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft

namespace net::minecraft::item {

class EggItem : public Item {
public:
    explicit EggItem(int rawId);
    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override;
};

} // namespace net::minecraft::item
