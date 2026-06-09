#pragma once

#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

class PaintingItem : public Item {
public:
    explicit PaintingItem(int rawId) : Item(rawId) {}

    bool useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int side) override
    {
        if (world == nullptr || stack == nullptr || side == 0 || side == 1) {
            return false;
        }
        int facing = 0;
        if (side == 4) {
            facing = 1;
        } else if (side == 3) {
            facing = 2;
        } else if (side == 5) {
            facing = 3;
        }
        if (!world->isRemote()) {
            auto* painting = new entity::decoration::painting::PaintingEntity(world);
            painting->facing = facing;
            painting->setPosition(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z));
            world->spawnEntity(painting);
        }
        --stack->count;
        return true;
    }
};

} // namespace net::minecraft::item
