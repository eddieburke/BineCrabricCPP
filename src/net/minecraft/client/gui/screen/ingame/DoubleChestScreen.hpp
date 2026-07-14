#pragma once
#include <memory>
#include <string>
#include "net/minecraft/client/gui/screen/ingame/HandledScreen.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
namespace net::minecraft::entity::player {
class PlayerInventory;
}
namespace net::minecraft::client::gui::screen::ingame {
class DoubleChestScreen : public HandledScreen {
public:
  DoubleChestScreen(entity::player::PlayerInventory* playerInventory, Inventory* inventory);

protected:
  void drawForeground() override;
  void drawBackground(float tickDelta) override;
  [[nodiscard]] std::string_view getScreenUiId() const override {
    return net::minecraft::mod::screen_ids::kDoubleChest;
  }

private:
  entity::player::PlayerInventory* playerInventory_ = nullptr;
  Inventory* inventory_ = nullptr;
  int rows_ = 0;
  std::unique_ptr<::net::minecraft::screen::ScreenHandler> ownedHandler_;
};
} // namespace net::minecraft::client::gui::screen::ingame
