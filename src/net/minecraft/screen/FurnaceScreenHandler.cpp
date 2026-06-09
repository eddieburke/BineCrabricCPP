#include "net/minecraft/screen/FurnaceScreenHandler.hpp"

#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"

namespace net::minecraft::screen {

FurnaceScreenHandler::FurnaceScreenHandler(PlayerInventory* playerInventory, Inventory* inventory)
    : inventory_(inventory),
      furnace_(dynamic_cast<block::entity::FurnaceBlockEntity*>(inventory))
{
    if (playerInventory == nullptr || inventory_ == nullptr) {
        return;
    }
    addSlot(new slot::Slot(inventory_, 0, 56, 17));
    addSlot(new slot::Slot(inventory_, 1, 56, 53));
    addSlot(new slot::FurnaceOutputSlot(playerInventory->player(), inventory_, 2, 116, 35));
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 9; ++column) {
            addSlot(new slot::Slot(playerInventory, column + row * 9 + 9, 8 + column * 18, 84 + row * 18));
        }
    }
    for (int column = 0; column < 9; ++column) {
        addSlot(new slot::Slot(playerInventory, column, 8 + column * 18, 142));
    }
}

void FurnaceScreenHandler::addListener(ScreenHandlerListener* listener)
{
    ScreenHandler::addListener(listener);
    if (listener == nullptr || furnace_ == nullptr) {
        return;
    }
    listener->onPropertyUpdate(this, 0, furnace_->cookTime);
    listener->onPropertyUpdate(this, 1, furnace_->burnTime);
    listener->onPropertyUpdate(this, 2, furnace_->fuelTime);
}

bool FurnaceScreenHandler::canUse(PlayerEntity* player)
{
    return inventory_ != nullptr && inventory_->canPlayerUse(player);
}

ItemStack FurnaceScreenHandler::quickMove(int slotIndex)
{
    ItemStack moved {};
    slot::Slot* currentSlot = getSlot(slotIndex);
    if (currentSlot == nullptr || !currentSlot->hasStack()) {
        return {};
    }
    ItemStack slotStack = currentSlot->getStack();
    moved = slotStack.copy();
    if (slotIndex == 2) {
        insertItem(slotStack, 3, 39, true);
    } else if (slotIndex >= 3 && slotIndex < 30) {
        insertItem(slotStack, 30, 39, false);
    } else if (slotIndex >= 30 && slotIndex < 39) {
        insertItem(slotStack, 3, 30, false);
    } else {
        insertItem(slotStack, 3, 39, false);
    }
    if (slotStack.count == 0) {
        currentSlot->setStack({});
    } else {
        currentSlot->setStack(slotStack);
    }
    if (slotStack.count != moved.count) {
        currentSlot->onTakeItem(slotStack);
        return moved;
    }
    return {};
}

void FurnaceScreenHandler::sendContentUpdates()
{
    ScreenHandler::sendContentUpdates();
    if (furnace_ == nullptr) {
        return;
    }
    for (ScreenHandlerListener* listener : listeners_) {
        if (listener == nullptr) {
            continue;
        }
        if (trackedCookTime_ != furnace_->cookTime) {
            listener->onPropertyUpdate(this, 0, furnace_->cookTime);
        }
        if (trackedBurnTime_ != furnace_->burnTime) {
            listener->onPropertyUpdate(this, 1, furnace_->burnTime);
        }
        if (trackedFuelTime_ != furnace_->fuelTime) {
            listener->onPropertyUpdate(this, 2, furnace_->fuelTime);
        }
    }
    trackedCookTime_ = furnace_->cookTime;
    trackedBurnTime_ = furnace_->burnTime;
    trackedFuelTime_ = furnace_->fuelTime;
}

void FurnaceScreenHandler::setProperty(int id, int value)
{
    if (furnace_ == nullptr) {
        return;
    }
    if (id == 0) {
        furnace_->cookTime = value;
    } else if (id == 1) {
        furnace_->burnTime = value;
    } else if (id == 2) {
        furnace_->fuelTime = value;
    }
}

} // namespace net::minecraft::screen
