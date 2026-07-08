#pragma once
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"

namespace net::minecraft::screen {
class GenericContainerScreenHandler : public ScreenHandler {
   public:
    GenericContainerScreenHandler(PlayerInventory* playerInventory, Inventory* inventory)
        : inventory_(inventory), rows(inventory != nullptr ? static_cast<int>(inventory->size() / 9) : 0) {
        if (playerInventory == nullptr || inventory_ == nullptr) {
            return;
        }
        for (int row = 0; row < rows; ++row) {
            for (int column = 0; column < 9; ++column) {
                addSlot(new slot::Slot(inventory_, column + row * 9, 8 + column * 18, 18 + row * 18));
            }
        }
        addPlayerInventorySlots(playerInventory, 8, 103 + (rows - 4) * 18, 161 + (rows - 4) * 18);
        setupStorageQuickMove(rows * 9);
    }

    [[nodiscard]] bool canUse(PlayerEntity* player) override {
        return inventory_ != nullptr && inventory_->canPlayerUse(player);
    }

   private:
    Inventory* inventory_ = nullptr;
    int rows = 0;
};
}  // namespace net::minecraft::screen
