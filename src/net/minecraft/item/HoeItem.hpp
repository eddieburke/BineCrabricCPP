#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemPlacement.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class HoeItem : public Item {
public:
    HoeItem(int rawId, ToolMaterial material)
        : Item(rawId)
    {
        setMaxCount(1);
        setMaxDamage(toolMaterialDurability(material));
    }

    bool useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int side) override
    {
        if (stack == nullptr || world == nullptr || Block::FARMLAND == nullptr) {
            return false;
        }
        const int blockId = world->getBlockId(x, y, z);
        const int above = world->getBlockId(x, y + 1, z);
        if ((side != 0 && above == 0 && blockId == (Block::GRASS_BLOCK != nullptr ? Block::GRASS_BLOCK->id : -1))
            || blockId == (Block::DIRT != nullptr ? Block::DIRT->id : -1)) {
            detail::playPlaceSound(world, Block::FARMLAND, x, y, z);
            if (world->isRemote()) {
                return true;
            }
            world->setBlock(x, y, z, Block::FARMLAND->id);
            stack->applyDamage(1);
            return true;
        }
        return false;
    }
};

} // namespace net::minecraft::item
