#pragma once

#include "net/minecraft/inventory/CraftingInventory.hpp"
#include "net/minecraft/inventory/CraftingResultInventory.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"

namespace net::minecraft::entity::player {
class PlayerInventory;
}

namespace net::minecraft::screen {

class PlayerScreenHandler : public ScreenHandler {
public:
    PlayerScreenHandler(entity::player::PlayerInventory* inventory, bool isLocalIn = true);

    void onSlotUpdate(Inventory* inventory) override;
    void onClosed(PlayerEntity* player) override;
    [[nodiscard]] bool canUse(PlayerEntity* player) override;
    [[nodiscard]] ItemStack quickMove(int slotIndex) override;

    CraftingInventory craftingInput;
    CraftingResultInventory craftingResult {};
    bool isLocal = false;
};

} // namespace net::minecraft::screen
