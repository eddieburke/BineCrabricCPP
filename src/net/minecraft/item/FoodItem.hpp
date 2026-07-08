#pragma once
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft {
class World;
class ItemStack;
}  // namespace net::minecraft

namespace net::minecraft::item {
class FoodItem : public Item {
   public:
    FoodItem(int rawId, int healAmount, bool wolfFood);
    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override;

    [[nodiscard]] int getHealthRestored() const {
        return healAmount_;
    }

    [[nodiscard]] bool isMeat() const {
        return wolfFood_;
    }

   protected:
    int healAmount_ = 0;
    bool wolfFood_ = false;
};
}  // namespace net::minecraft::item
