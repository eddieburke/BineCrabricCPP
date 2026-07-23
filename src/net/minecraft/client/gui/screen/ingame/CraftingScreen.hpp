#pragma once
#include <memory>
#include "net/minecraft/client/gui/screen/ingame/HandledScreen.hpp"
namespace net::minecraft::entity::player {
class PlayerInventory;
}
namespace net::minecraft::screen {
class CraftingScreenHandler;
}
namespace net::minecraft::client::gui::screen::ingame {
class CraftingScreen : public HandledScreen {
 public:
 CraftingScreen(entity::player::PlayerInventory* playerInventory, int x, int y, int z);
 void removed() override;

 protected:
 void drawForeground() override;
 void drawBackground(float tickDelta) override;
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kCrafting;
 }

 private:
 std::unique_ptr<::net::minecraft::screen::CraftingScreenHandler> ownedHandler_;
};
} // namespace net::minecraft::client::gui::screen::ingame
