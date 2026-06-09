#pragma once

#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

#include <memory>

namespace net::minecraft::item {

class BowItem : public Item {
public:
    explicit BowItem(int rawId) : Item(rawId)
    {
        setMaxCount(1);
    }

    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override
    {
        if (world == nullptr || user == nullptr) {
            return stack;
        }
        if (user->inventory.remove(Item::ARROW != nullptr ? Item::ARROW->id : 262)) {
            world->playSound(user, "random.bow", 1.0f, 1.0f / (random.nextFloat() * 0.4f + 0.8f));
            if (!world->isRemote()) {
                auto projectile = std::make_unique<entity::projectile::ArrowEntity>(world, user);
                if (world->spawnEntity(projectile.get())) {
                    projectile.release();
                }
            }
        }
        return stack;
    }
};

} // namespace net::minecraft::item
