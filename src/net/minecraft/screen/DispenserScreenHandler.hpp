#pragma once
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"

namespace net::minecraft::screen {
class DispenserScreenHandler : public ScreenHandler {
   public:
    DispenserScreenHandler(PlayerInventory* playerInventory, Inventory* inventory) : inventory_(inventory) {
        if (playerInventory == nullptr || inventory_ == nullptr) {
            return;
        }
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                addSlot(new slot::Slot(inventory_, column + row * 3, 62 + column * 18, 17 + row * 18));
            }
        }
        addPlayerInventorySlots(playerInventory);
        setupStorageQuickMove(9);
    }

    [[nodiscard]] bool canUse(PlayerEntity* player) override {
        return inventory_ != nullptr && inventory_->canPlayerUse(player);
    }

   private:
    Inventory* inventory_ = nullptr;
};
}  // namespace net::minecraft::screen
