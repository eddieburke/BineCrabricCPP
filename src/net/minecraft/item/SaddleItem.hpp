#pragma once

#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class SaddleItem : public Item {
public:
    static void registerClass();
    explicit SaddleItem(int rawId);
    void useOnEntity(ItemStack* stack, LivingEntity* entity) override;
    bool postHit(ItemStack* stack, LivingEntity* target, LivingEntity* attacker) override;
};

} // namespace net::minecraft::item
