#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::host {
class LanInfoScreen : public gui::screen::Screen {
public:
  void init() override;
  void render(int mouseX, int mouseY, float tickDelta) override;

protected:
  void keyPressed(char character, int keyCode) override;
  [[nodiscard]] std::string_view getScreenUiId() const override {
    return net::minecraft::mod::screen_ids::kLanInfo;
  }

private:
  void backToGameMenu();
};
} // namespace net::minecraft::client::host
