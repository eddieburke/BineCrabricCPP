#pragma once
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen::world {
class WorldSaveConflictScreen : public screen::Screen {
 public:
 void tick() override {
  ++ticksRan_;
 }
 void init() override {
  buttons_.clear();
  addCenteredActionButton(layout::dialogFooterY(height_), "Back to title screen", [this] { quitToTitle(); });
 }
 void render(int mouseX, int mouseY, float delta) override {
  renderBackground();
  if(textRenderer() != nullptr) {
   drawCenteredTextWithShadow(
       *textRenderer(), "Level save conflict", width_ / 2, height_ / 4 - 60 + 20, 0xFFFFFF);
   drawTextWithShadow(*textRenderer(),
                      "Minecraft detected a conflict in the level save data.",
                      width_ / 2 - 140,
                      height_ / 4 - 60 + 60 + 0,
                      0xA0A0A0);
   drawTextWithShadow(*textRenderer(),
                      "This could be caused by two copies of the game",
                      width_ / 2 - 140,
                      height_ / 4 - 60 + 60 + 18,
                      0xA0A0A0);
   drawTextWithShadow(
       *textRenderer(), "accessing the same level.", width_ / 2 - 140, height_ / 4 - 60 + 60 + 27, 0xA0A0A0);
   drawTextWithShadow(*textRenderer(),
                      "To prevent level corruption, the current game has quit.",
                      width_ / 2 - 140,
                      height_ / 4 - 60 + 60 + 45,
                      0xA0A0A0);
  }
  screen::Screen::render(mouseX, mouseY, delta);
 }
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kWorldSaveConflict;
 }

 private:
 int ticksRan_ = 0;
};
} // namespace net::minecraft::client::gui::screen::world
