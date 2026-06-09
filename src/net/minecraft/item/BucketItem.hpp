#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft

namespace net::minecraft::item {

class BucketItem : public Item {
public:
    BucketItem(int rawId, int fluidBlockId);
    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override;

private:
    int fluidBlockId_ = 0;
};

} // namespace net::minecraft::item
