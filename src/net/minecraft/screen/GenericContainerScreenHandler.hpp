#pragma once

#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"

namespace net::minecraft::screen {

class GenericContainerScreenHandler : public ScreenHandler {
public:
    GenericContainerScreenHandler(PlayerInventory* playerInventory, Inventory* inventory)
        : inventory_(inventory),
          rows(inventory != nullptr ? static_cast<int>(inventory->size() / 9) : 0)
    {
        if (playerInventory == nullptr || inventory_ == nullptr) {
            return;
        }
        for (int row = 0; row < rows; ++row) {
            for (int column = 0; column < 9; ++column) {
                addSlot(new slot::Slot(inventory_, column + row * 9, 8 + column * 18, 18 + row * 18));
            }
        }
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 9; ++column) {
                addSlot(new slot::Slot(playerInventory, column + row * 9 + 9, 8 + column * 18, 103 + row * 18 + (rows - 4) * 18));
            }
        }
        for (int column = 0; column < 9; ++column) {
            addSlot(new slot::Slot(playerInventory, column, 8 + column * 18, 161 + (rows - 4) * 18));
        }
    }

    [[nodiscard]] bool canUse(PlayerEntity* player) override
    {
        return inventory_ != nullptr && inventory_->canPlayerUse(player);
    }

    [[nodiscard]] ItemStack quickMove(int slotIndex) override
    {
        ItemStack moved {};
        slot::Slot* currentSlot = getSlot(slotIndex);
        if (currentSlot == nullptr || !currentSlot->hasStack()) {
            return {};
        }
        ItemStack slotStack = currentSlot->getStack();
        moved = slotStack.copy();
        const int containerSlots = rows * 9;
        if (slotIndex < containerSlots) {
            insertItem(slotStack, containerSlots, static_cast<int>(slots.size()), true);
        } else {
            insertItem(slotStack, 0, containerSlots, false);
        }
        if (slotStack.count == 0) {
            currentSlot->setStack({});
        } else {
            currentSlot->setStack(slotStack);
        }
        return moved;
    }

private:
    Inventory* inventory_ = nullptr;
    int rows = 0;
};

} // namespace net::minecraft::screen
