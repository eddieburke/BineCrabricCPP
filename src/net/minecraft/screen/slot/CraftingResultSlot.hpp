#pragma once
#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft::screen::slot {
class CraftingResultSlot : public Slot {
 public:
 CraftingResultSlot(
     entity::player::PlayerEntity* player, Inventory* input, Inventory* inventory, int index, int x, int y)
     : Slot(inventory, index, x, y), player_(player), input_(input) {
 }
 [[nodiscard]] bool canInsert(const ItemStack& stack) const override {
  (void)stack;
  return false;
 }
 void onTakeItem(const ItemStack& stack) override {
  // no Lua hook
  if(input_ == nullptr) {
   return;
  }
  if(player_ != nullptr && player_->world != nullptr) {
   ItemStack crafted = stack;
   crafted.onCraft(player_->world, player_);
   if(Block::CRAFTING_TABLE != nullptr && stack.itemId == Block::CRAFTING_TABLE->id) {
    player_->increaseStat(achievement::Achievements::CRAFT_WORKBENCH.statId(), 1);
   } else if(Item::byRawId(14) != nullptr && stack.itemId == Item::byRawId(14)->id) {
    player_->increaseStat(achievement::Achievements::CRAFT_PICKAXE.statId(), 1);
   } else if(Block::FURNACE != nullptr && stack.itemId == Block::FURNACE->id) {
    player_->increaseStat(achievement::Achievements::CRAFT_FURNACE.statId(), 1);
   } else if(Item::byRawId(34) != nullptr && stack.itemId == Item::byRawId(34)->id) {
    player_->increaseStat(achievement::Achievements::CRAFT_HOE.statId(), 1);
   } else if(Item::byRawId(41) != nullptr && stack.itemId == Item::byRawId(41)->id) {
    player_->increaseStat(achievement::Achievements::CRAFT_BREAD.statId(), 1);
   } else if(Item::byRawId(98) != nullptr && stack.itemId == Item::byRawId(98)->id) {
    player_->increaseStat(achievement::Achievements::CRAFT_CAKE.statId(), 1);
   } else if(Item::byRawId(18) != nullptr && stack.itemId == Item::byRawId(18)->id) {
    player_->increaseStat(achievement::Achievements::CRAFT_STONE_PICKAXE.statId(), 1);
   } else if(Item::byRawId(12) != nullptr && stack.itemId == Item::byRawId(12)->id) {
    player_->increaseStat(achievement::Achievements::CRAFT_SWORD.statId(), 1);
   }
  }
  for(std::size_t i = 0; i < input_->size(); ++i) {
   ItemStack ingredient = input_->getStack(i);
   if(ingredient.empty()) {
    continue;
   }
   input_->removeStack(i, 1);
   Item* item = ingredient.getItem();
   if(item != nullptr && item->hasCraftingReturnItem()) {
    input_->setStack(i, ItemStack(item->getCraftingReturnItem()));
   }
  }
 }

 private:
 entity::player::PlayerEntity* player_ = nullptr;
 Inventory* input_ = nullptr;
};
} // namespace net::minecraft::screen::slot
