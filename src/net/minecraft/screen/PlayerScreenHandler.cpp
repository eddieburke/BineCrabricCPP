#include "net/minecraft/screen/PlayerScreenHandler.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/item/ArmorItem.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/screen/slot/CraftingResultSlot.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"
namespace net::minecraft::screen {
namespace {
class ArmorSlot : public slot::Slot {
 public:
 ArmorSlot(
     PlayerScreenHandler* handler, entity::player::PlayerInventory* inventory, int armorSlotIndex, int x, int y)
     : Slot(inventory, static_cast<int>(inventory->size()) - 1 - armorSlotIndex, x, y),
       armorSlotIndex_(armorSlotIndex) {
  (void)handler;
 }
 [[nodiscard]] int getMaxItemCount() const override {
  return 1;
 }
 [[nodiscard]] bool canInsert(const ItemStack& stack) const override {
  Item* item = stack.getItem();
  if(auto* armor = dynamic_cast<item::ArmorItem*>(item); armor != nullptr) {
   return armor->getEquipmentSlot() == armorSlotIndex_;
  }
  if(Block::PUMPKIN != nullptr && stack.itemId == Block::PUMPKIN->id) {
   return armorSlotIndex_ == 0;
  }
  return false;
 }

 private:
 int armorSlotIndex_ = 0;
};
} // namespace
PlayerScreenHandler::PlayerScreenHandler(entity::player::PlayerInventory* inventory, bool isLocalIn)
    : craftingInput(this, 2, 2), isLocal(isLocalIn) {
 if(inventory == nullptr) {
  return;
 }
 entity::player::PlayerEntity* player = inventory->player();
 addSlot(new slot::CraftingResultSlot(player, &craftingInput, &craftingResult, 0, 144, 36));
 for(int row = 0; row < 2; ++row) {
  for(int column = 0; column < 2; ++column) {
   addSlot(new slot::Slot(&craftingInput, column + row * 2, 88 + column * 18, 26 + row * 18));
  }
 }
 for(int armorSlot = 0; armorSlot < 4; ++armorSlot) {
  addSlot(new ArmorSlot(this, inventory, armorSlot, 8, 8 + armorSlot * 18));
 }
 addPlayerInventorySlots(inventory);
 setupQuickMove(9, {0}); // player inv starts at slot 9; slot 0 is the craft result
 onSlotUpdate(&craftingInput);
}
void PlayerScreenHandler::onSlotUpdate(Inventory* inventory) {
 (void)inventory;
 craftingResult.setStack(0, recipe::CraftingRecipeManager::getInstance().craft(craftingInput));
}
void PlayerScreenHandler::onClosed(PlayerEntity* player) {
 ScreenHandler::onClosed(player);
 if(player == nullptr) {
  return;
 }
 for(std::size_t i = 0; i < craftingInput.size(); ++i) {
  ItemStack stack = craftingInput.getStack(i);
  if(stack.empty()) {
   continue;
  }
  player->dropItem(stack);
  craftingInput.setStack(i, {});
 }
}
bool PlayerScreenHandler::canUse(PlayerEntity* player) {
 (void)player;
 return true;
}
} // namespace net::minecraft::screen
