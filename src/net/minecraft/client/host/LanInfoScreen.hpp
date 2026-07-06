#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::host {
class LanInfoScreen : public gui::screen::Screen {
public:
  void init() override;
  void render(int mouseX, int mouseY, float tickDelta) override;

protected:
  void keyPressed(char character, int keyCode) override;

private:
  void backToGameMenu();
};
} // namespace net::minecraft::client::host
