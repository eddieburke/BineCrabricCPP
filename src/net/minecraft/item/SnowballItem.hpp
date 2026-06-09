#pragma once

#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

#include <memory>

namespace net::minecraft::item {

class SnowballItem : public Item {
public:
    explicit SnowballItem(int rawId) : Item(rawId)
    {
        setMaxCount(16);
    }

    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override
    {
        if (stack == nullptr || stack->count <= 0) {
            return stack;
        }
        --stack->count;
        if (world != nullptr && user != nullptr) {
            world->playSound(user, "random.bow", 0.5f, 0.4f / (random.nextFloat() * 0.4f + 0.8f));
            if (!world->isRemote()) {
                auto projectile = std::make_unique<entity::projectile::thrown::SnowballEntity>(world, user);
                if (world->spawnEntity(projectile.get())) {
                    projectile.release();
                }
            }
        }
        return stack;
    }
};

} // namespace net::minecraft::item
