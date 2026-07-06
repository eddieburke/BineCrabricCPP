#include "net/minecraft/screen/CraftingScreenHandler.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::screen {
CraftingScreenHandler::CraftingScreenHandler(entity::player::PlayerInventory* inventory, int xIn, int yIn, int zIn)
    : craftingInput(this, 3, 3), x(xIn), y(yIn), z(zIn) {
  if(inventory == nullptr) {
    return;
  }
  addSlot(new slot::CraftingResultSlot(inventory->player(), &craftingInput, &craftingResult, 0, 124, 35));
  for(int row = 0; row < 3; ++row) {
    for(int column = 0; column < 3; ++column) {
      addSlot(new slot::Slot(&craftingInput, column + row * 3, 30 + column * 18, 17 + row * 18));
    }
  }
  addPlayerInventorySlots(inventory);
  setupQuickMove(10, {0}); // player inv starts at slot 10; slot 0 is the craft result
  onSlotUpdate(&craftingInput);
}
void CraftingScreenHandler::onSlotUpdate(Inventory* inventory) {
  (void)inventory;
  craftingResult.setStack(0, recipe::CraftingRecipeManager::getInstance().craft(craftingInput));
}
void CraftingScreenHandler::onClosed(PlayerEntity* player) {
  ScreenHandler::onClosed(player);
  if(player == nullptr || player->world == nullptr || player->world->isRemote()) {
    return;
  }
  for(std::size_t i = 0; i < craftingInput.size(); ++i) {
    ItemStack stack = craftingInput.getStack(i);
    if(stack.empty()) {
      continue;
    }
    player->dropItem(stack);
  }
}
bool CraftingScreenHandler::canUse(PlayerEntity* player) {
  if(player == nullptr || player->world == nullptr) {
    return false;
  }
  if(Block::CRAFTING_TABLE == nullptr || player->world->getBlockId(x, y, z) != Block::CRAFTING_TABLE->id) {
    return false;
  }
  return player->getSquaredDistance(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5,
                                    static_cast<double>(z) + 0.5) <= 64.0;
}
} // namespace net::minecraft::screen
