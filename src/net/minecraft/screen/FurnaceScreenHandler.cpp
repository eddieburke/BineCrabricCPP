#include "net/minecraft/screen/FurnaceScreenHandler.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"
namespace net::minecraft::screen {
FurnaceScreenHandler::FurnaceScreenHandler(PlayerInventory* playerInventory, Inventory* inventory)
    : inventory_(inventory), furnace_(dynamic_cast<block::entity::FurnaceBlockEntity*>(inventory)) {
  if(playerInventory == nullptr || inventory_ == nullptr) {
    return;
  }
  addSlot(new slot::Slot(inventory_, 0, 56, 17));
  addSlot(new slot::Slot(inventory_, 1, 56, 53));
  addSlot(new slot::FurnaceOutputSlot(playerInventory->player(), inventory_, 2, 116, 35));
  addPlayerInventorySlots(playerInventory);
  setupQuickMove(3, {2}); // player inv starts at slot 3; slot 2 is the output
}
bool FurnaceScreenHandler::canUse(PlayerEntity* player) {
  return inventory_ != nullptr && inventory_->canPlayerUse(player);
}
void FurnaceScreenHandler::addListener(ScreenHandlerListener* listener) {
  ScreenHandler::addListener(listener);
  if(listener == nullptr || furnace_ == nullptr) {
    return;
  }
  listener->onPropertyUpdate(*this, 0, furnace_->cookTime);
  listener->onPropertyUpdate(*this, 1, furnace_->burnTime);
  listener->onPropertyUpdate(*this, 2, furnace_->fuelTime);
}
void FurnaceScreenHandler::sendContentUpdates() {
  ScreenHandler::sendContentUpdates();
  if(furnace_ == nullptr) {
    return;
  }
  for(ScreenHandlerListener* listener : listeners_) {
    if(listener == nullptr) {
      continue;
    }
    if(cookTime_ != furnace_->cookTime) {
      listener->onPropertyUpdate(*this, 0, furnace_->cookTime);
    }
    if(burnTime_ != furnace_->burnTime) {
      listener->onPropertyUpdate(*this, 1, furnace_->burnTime);
    }
    if(fuelTime_ != furnace_->fuelTime) {
      listener->onPropertyUpdate(*this, 2, furnace_->fuelTime);
    }
  }
  cookTime_ = furnace_->cookTime;
  burnTime_ = furnace_->burnTime;
  fuelTime_ = furnace_->fuelTime;
}
void FurnaceScreenHandler::setProperty(int id, int value) {
  if(furnace_ == nullptr) {
    return;
  }
  if(id == 0) {
    furnace_->cookTime = value;
  } else if(id == 1) {
    furnace_->burnTime = value;
  } else if(id == 2) {
    furnace_->fuelTime = value;
  }
}
} // namespace net::minecraft::screen
