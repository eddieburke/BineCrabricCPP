#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include <string>
namespace net::minecraft::client::gui::screen {
class ChatScreen : public Screen {
public:
  void init() override;
  void removed() override;
  void tick() override;
  void render(int mouseX, int mouseY, float tickDelta) override;
  void keyPressed(char character, int keyCode) override;
  void mouseClicked(int mouseX, int mouseY, int button) override;

protected:
  std::string text_;

private:
  int focusedTicks_ = 0;
};
} // namespace net::minecraft::client::gui::screen
