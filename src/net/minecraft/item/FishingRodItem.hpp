#pragma once

#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

#include <memory>

namespace net::minecraft::item {

class FishingRodItem : public Item {
public:
    explicit FishingRodItem(int rawId) : Item(rawId)
    {
        setMaxCount(1);
        setMaxDamage(64);
    }

    [[nodiscard]] bool isHandheldRod() const override { return true; }

    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override
    {
        if (world == nullptr || user == nullptr) {
            return stack;
        }
        if (user->fishHook != nullptr) {
            const int damage = user->fishHook->use();
            if (stack != nullptr) {
                stack->applyDamage(damage);
            }
            user->swingHand();
        } else {
            world->playSound(user, "random.bow", 0.5f, 0.4f / (random.nextFloat() * 0.4f + 0.8f));
            if (!world->isRemote()) {
                auto bobber = std::make_unique<entity::projectile::FishingBobberEntity>(world, user);
                if (world->spawnEntity(bobber.get())) {
                    bobber.release();
                }
            }
            user->swingHand();
        }
        return stack;
    }
};

} // namespace net::minecraft::item
