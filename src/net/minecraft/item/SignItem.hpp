#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

class SignItem : public Item {
public:
    explicit SignItem(int rawId) : Item(rawId)
    {
        setMaxCount(1);
    }

    bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override
    {
        if (stack == nullptr || user == nullptr || world == nullptr || side == 0 || Block::SIGN == nullptr || Block::WALL_SIGN == nullptr) {
            return false;
        }
        if (!world->getMaterial(x, y, z).isSolid()) {
            return false;
        }
        if (side == 1) {
            ++y;
        } else if (side == 2) {
            --z;
        } else if (side == 3) {
            ++z;
        } else if (side == 4) {
            --x;
        } else if (side == 5) {
            ++x;
        }
        if (!Block::SIGN->canPlaceAt(world, x, y, z)) {
            return false;
        }
        if (side == 1) {
            const int meta = MathHelper::floor((user->yaw + 180.0f) * 16.0f / 360.0f + 0.5f) & 0xF;
            world->setBlock(x, y, z, Block::SIGN->id, static_cast<std::uint8_t>(meta));
        } else {
            world->setBlock(x, y, z, Block::WALL_SIGN->id, static_cast<std::uint8_t>(side));
        }
        --stack->count;
        return true;
    }
};

} // namespace net::minecraft::item
