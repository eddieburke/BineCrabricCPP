#pragma once
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/client/gui/screen/ingame/HandledScreen.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
#include <memory>
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

private:
  block::entity::DispenserBlockEntity* dispenser_ = nullptr;
  std::unique_ptr<::net::minecraft::screen::ScreenHandler> ownedHandler_;
};
} // namespace net::minecraft::client::gui::screen::ingame
