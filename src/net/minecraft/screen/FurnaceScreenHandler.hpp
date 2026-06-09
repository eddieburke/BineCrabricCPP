#pragma once

#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
#include "net/minecraft/screen/slot/FurnaceOutputSlot.hpp"

namespace net::minecraft::block::entity {
class FurnaceBlockEntity;
}

namespace net::minecraft::screen {

class FurnaceScreenHandler : public ScreenHandler {
public:
    FurnaceScreenHandler(PlayerInventory* playerInventory, Inventory* inventory);

    void addListener(ScreenHandlerListener* listener);
    [[nodiscard]] bool canUse(PlayerEntity* player) override;
    [[nodiscard]] ItemStack quickMove(int slotIndex) override;
    void sendContentUpdates() override;
    void setProperty(int id, int value) override;

private:
    Inventory* inventory_ = nullptr;
    block::entity::FurnaceBlockEntity* furnace_ = nullptr;
    int trackedCookTime_ = 0;
    int trackedBurnTime_ = 0;
    int trackedFuelTime_ = 0;
};

} // namespace net::minecraft::screen
