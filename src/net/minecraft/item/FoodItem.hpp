#pragma once

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::item {

class FoodItem : public Item {
public:
    FoodItem(int rawId, int healAmount, bool wolfFood)
        : Item(rawId),
          healAmount_(healAmount),
          wolfFood_(wolfFood)
    {
        setMaxCount(1);
    }

    ItemStack* use(ItemStack* stack, World* /*world*/, PlayerEntity* user) override
    {
        if (stack != nullptr) {
            --stack->count;
        }
        if (user != nullptr) {
            user->heal(healAmount_);
        }
        return stack;
    }

    [[nodiscard]] int getHealthRestored() const { return healAmount_; }
    [[nodiscard]] bool isMeat() const { return wolfFood_; }

protected:
    int healAmount_ = 0;
    bool wolfFood_ = false;
};

} // namespace net::minecraft::item
