#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemPlacement.hpp"

namespace net::minecraft::item {

class RedstoneItem : public Item {
public:
    explicit RedstoneItem(int rawId) : Item(rawId) {}

    bool useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int side) override
    {
        if (world == nullptr || stack == nullptr || Block::REDSTONE_WIRE == nullptr) {
            return false;
        }
        if (Block::SNOW == nullptr || world->getBlockId(x, y, z) != Block::SNOW->id) {
            detail::offsetPlacementPos(world, x, y, z, side);
            if (!world->isAir(x, y, z)) {
                return false;
            }
        }
        if (Block::REDSTONE_WIRE->canPlaceAt(world, x, y, z)) {
            --stack->count;
            world->setBlock(x, y, z, Block::REDSTONE_WIRE->id);
        }
        return true;
    }
};

} // namespace net::minecraft::item
