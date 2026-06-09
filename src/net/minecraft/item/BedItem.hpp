#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft

namespace net::minecraft::item {

class BedItem : public Item {
public:
    explicit BedItem(int rawId);
    bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override;
};

} // namespace net::minecraft::item
