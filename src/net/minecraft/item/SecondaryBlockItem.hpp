#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemPlacement.hpp"

namespace net::minecraft::item {

class SecondaryBlockItem : public Item {
public:
    SecondaryBlockItem(int rawId, Block* block)
        : Item(rawId),
          blockId_(block != nullptr ? block->id : 0)
    {
    }

    bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override
    {
        return detail::placeBlockItem(stack, user, world, blockId_, 0, x, y, z, side, true);
    }

private:
    int blockId_ = 0;
};

} // namespace net::minecraft::item
