#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen {
class DeathScreen : public Screen {
 public:
 void init() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 void keyPressed(char character, int keyCode) override;
 [[nodiscard]] bool shouldPause() const override {
  return false;
 }
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kDeath;
 }
};
} // namespace net::minecraft::client::gui::screen
