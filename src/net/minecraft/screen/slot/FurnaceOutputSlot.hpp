#pragma once

#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/food/cooked_fish.hpp"
#include "net/minecraft/item/misc/iron_ingot.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"


namespace net::minecraft::screen::slot {

class FurnaceOutputSlot : public Slot {
public:
    FurnaceOutputSlot(entity::player::PlayerEntity* player, Inventory* inventory, int index, int x, int y)
        : Slot(inventory, index, x, y),
          player_(player)
    {
    }

    [[nodiscard]] bool canInsert(const ItemStack& stack) const override
    {
        (void)stack;
        return false;
    }

    void onTakeItem(const ItemStack& stack) override
    {
        if (player_ != nullptr) {
            ItemStack crafted = stack;
            crafted.onCraft(player_->world, player_);
            if (Item::byRawId(9) != nullptr && stack.itemId == Item::byRawId(9)->id) {
                player_->increaseStat(achievement::Achievements::ACQUIRE_IRON.statId(), 1);
            }
            if (Item::byRawId(94) != nullptr && stack.itemId == Item::byRawId(94)->id) {
                player_->increaseStat(achievement::Achievements::COOK_FISH.statId(), 1);
            }
        }
        Slot::onTakeItem(stack);
    }

private:
    entity::player::PlayerEntity* player_ = nullptr;
};

} // namespace net::minecraft::screen::slot
