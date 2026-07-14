#pragma once
#include <string>
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen {
class TitleScreen : public Screen {
public:
  TitleScreen();
  void init() override;
  void tick() override;
  void render(int mouseX, int mouseY, float tickDelta) override;
  void keyPressed(char character, int keyCode) override;
  [[nodiscard]] std::string_view getScreenUiId() const override {
    return net::minecraft::mod::screen_ids::kTitle;
  }

private:
  void applyCalendarSplash();
  std::string chooseSplashText() const;
  float ticks_ = 0.0f;
  std::string splashText_{};
  widget::ActionButtonWidget* multiplayerButton_ = nullptr;
  widget::ActionButtonWidget* accountButton_ = nullptr;
};
} // namespace net::minecraft::client::gui::screen
