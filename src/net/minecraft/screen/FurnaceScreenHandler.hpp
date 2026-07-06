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
  [[nodiscard]] bool canUse(PlayerEntity* player) override;
  void setProperty(int id, int value) override;

private:
  Inventory* inventory_ = nullptr;
  block::entity::FurnaceBlockEntity* furnace_ = nullptr;
};
} // namespace net::minecraft::screen
