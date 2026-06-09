#pragma once

#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/Item.hpp"
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
            if (Item::IRON_INGOT != nullptr && stack.itemId == Item::IRON_INGOT->id) {
                player_->increaseStat(achievement::Achievements::ACQUIRE_IRON.statId(), 1);
            }
            if (Item::COOKED_FISH != nullptr && stack.itemId == Item::COOKED_FISH->id) {
                player_->increaseStat(achievement::Achievements::COOK_FISH.statId(), 1);
            }
        }
        Slot::onTakeItem(stack);
    }

private:
    entity::player::PlayerEntity* player_ = nullptr;
};

} // namespace net::minecraft::screen::slot
