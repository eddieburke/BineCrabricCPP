#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/inventory/CraftingInventory.hpp"
#include "net/minecraft/inventory/CraftingResultInventory.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
#include "net/minecraft/screen/slot/CraftingResultSlot.hpp"

namespace net::minecraft::entity::player {
class PlayerEntity;
class PlayerInventory;
}  // namespace net::minecraft::entity::player

namespace net::minecraft::screen {
class CraftingScreenHandler : public ScreenHandler {
   public:
    CraftingScreenHandler(entity::player::PlayerInventory* inventory, int xIn = 0, int yIn = 0, int zIn = 0);
    void onSlotUpdate(Inventory* inventory) override;
    void onClosed(PlayerEntity* player) override;
    [[nodiscard]] bool canUse(PlayerEntity* player) override;
    CraftingInventory craftingInput;
    CraftingResultInventory craftingResult{};
    int x = 0;
    int y = 0;
    int z = 0;
};
}  // namespace net::minecraft::screen
