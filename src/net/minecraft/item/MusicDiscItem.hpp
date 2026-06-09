#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

#include <string>

namespace net::minecraft::item {

class MusicDiscItem : public Item {
public:
    MusicDiscItem(int rawId, std::string sound)
        : Item(rawId),
          sound(std::move(sound))
    {
        setMaxCount(1);
    }

    bool useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int /*side*/) override
    {
        if (world == nullptr || stack == nullptr || Block::JUKEBOX == nullptr) {
            return false;
        }
        if (world->getBlockId(x, y, z) == Block::JUKEBOX->id && world->getBlockMeta(x, y, z) == 0) {
            if (world->isRemote()) {
                return true;
            }
            world->setBlockMetaWithoutNotifyingNeighbors(x, y, z, 1);
            world->worldEvent(nullptr, 1005, x, y, z, id);
            --stack->count;
            return true;
        }
        return false;
    }

    std::string sound;
};

} // namespace net::minecraft::item
