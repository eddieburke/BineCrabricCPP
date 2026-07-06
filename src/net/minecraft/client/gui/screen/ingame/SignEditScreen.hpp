#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include <array>
#include <string>
namespace net::minecraft::client::gui::screen::ingame {
class SignEditScreen : public screen::Screen {
public:
  std::array<std::string, 4> text{};
  int currentLine = 0;
  void init() override;
  void render(int mouseX, int mouseY, float tickDelta) override;
  void keyPressed(char character, int keyCode) override;
};
} // namespace net::minecraft::client::gui::screen::ingame
