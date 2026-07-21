#pragma once
#include <memory>
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/client/gui/screen/ingame/HandledScreen.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
namespace net::minecraft::entity::player {
class PlayerInventory;
}
namespace net::minecraft::client::gui::screen::ingame {
class DispenserScreen : public HandledScreen {
 public:
 DispenserScreen(entity::player::PlayerInventory* playerInventory, block::entity::DispenserBlockEntity* dispenser);

 protected:
 void drawForeground() override;
 void drawBackground(float tickDelta) override;
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kDispenser;
 }

 private:
 block::entity::DispenserBlockEntity* dispenser_ = nullptr;
 std::unique_ptr<::net::minecraft::screen::ScreenHandler> ownedHandler_;
};
} // namespace net::minecraft::client::gui::screen::ingame
