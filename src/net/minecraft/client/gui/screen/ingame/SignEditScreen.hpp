#pragma once
#include <array>
#include <string>
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen::ingame {
class SignEditScreen : public screen::Screen {
 public:
 std::array<std::string, 4> text{};
 int currentLine = 0;
 void init() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 void keyPressed(char character, int keyCode) override;
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kSignEdit;
 }
};
} // namespace net::minecraft::client::gui::screen::ingame
