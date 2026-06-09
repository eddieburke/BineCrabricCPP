#pragma once

#include "net/minecraft/entity/passive/PigEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::item {

class SaddleItem : public Item {
public:
    explicit SaddleItem(int rawId) : Item(rawId)
    {
        setMaxCount(1);
    }

    void useOnEntity(ItemStack* stack, LivingEntity* entity) override
    {
        auto* pig = dynamic_cast<entity::passive::PigEntity*>(entity);
        if (stack != nullptr && pig != nullptr && !pig->isSaddled()) {
            pig->setSaddled(true);
            --stack->count;
        }
    }

    bool postHit(ItemStack* stack, LivingEntity* target, LivingEntity* /*attacker*/) override
    {
        useOnEntity(stack, target);
        return true;
    }
};

} // namespace net::minecraft::item
