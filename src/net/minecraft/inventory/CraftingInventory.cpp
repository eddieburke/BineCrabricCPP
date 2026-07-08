#include "net/minecraft/inventory/CraftingInventory.hpp"

#include "net/minecraft/screen/ScreenHandler.hpp"

namespace net::minecraft {
void CraftingInventory::notifySlotUpdate() {
    if (handler_ != nullptr) {
        handler_->onSlotUpdate(this);
    }
}
}  // namespace net::minecraft
