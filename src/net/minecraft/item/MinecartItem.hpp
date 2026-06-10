#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft

namespace net::minecraft::item {

class MinecartItem : public Item {
public:
    static void registerClass();
    MinecartItem(int rawId, int type);
    bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override;

private:
    int type_ = 0;
};

} // namespace net::minecraft::item
