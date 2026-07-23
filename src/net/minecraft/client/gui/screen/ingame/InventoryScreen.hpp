#pragma once
#include "net/minecraft/client/gui/screen/ingame/HandledScreen.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
namespace net::minecraft::client::gui::screen::ingame {
class InventoryScreen : public HandledScreen {
 public:
 explicit InventoryScreen(::net::minecraft::screen::ScreenHandler* handler)
     : HandledScreen(handler), inputScope_(input::InputLayer::ScreenPassthrough) {
  passEvents = true;
 }
 void init() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 void onMouseEvent() override;
 void mouseClicked(int mouseX, int mouseY, int button) override;
 [[nodiscard]] std::string_view getScreenUiId() const override;

 protected:
 void drawForeground() override;
 void drawBackground(float tickDelta) override;

 private:
 void publishSidePanelEvent(mod::ScreenRegionEvent& event) const;
 input::InputScope inputScope_;
 float mouseX_ = 0.0f;
 float mouseY_ = 0.0f;
};
} // namespace net::minecraft::client::gui::screen::ingame
