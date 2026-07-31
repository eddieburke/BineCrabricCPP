#pragma once
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen {
class OutOfMemoryScreen : public Screen {
 public:
 void tick() override {
  ++ticksRan_;
 }
 void init() override {
  buttons_.clear();
  addCenteredActionButton(layout::dialogButtonY(height_), "Quit game", [this] { quitGame(); });
 }
 void render(int mouseX, int mouseY, float tickDelta) override {
  renderBackground();
  if(textRenderer() != nullptr) {
   textRenderer()->drawCenteredWithShadow("Out of memory!", width_ / 2, height_ / 4 - 60 + 20, 0xFFFFFF);
   textRenderer()->drawWithShadow("Minecraft has run out of memory.",
                                  width_ / 2 - 140,
                                  height_ / 4 - 60 + 60 + 0,
                                  0xA0A0A0);
   textRenderer()->drawWithShadow("This could be caused by a bug in the game or by the",
                                  width_ / 2 - 140,
                                  height_ / 4 - 60 + 60 + 18,
                                  0xA0A0A0);
   textRenderer()->drawWithShadow("Java Virtual Machine not being allocated enough",
                                  width_ / 2 - 140,
                                  height_ / 4 - 60 + 60 + 27,
                                  0xA0A0A0);
   textRenderer()->drawWithShadow("memory. If you are playing in a web browser, try",
                                  width_ / 2 - 140,
                                  height_ / 4 - 60 + 60 + 36,
                                  0xA0A0A0);
   textRenderer()->drawWithShadow("downloading the game and playing it offline.",
                                  width_ / 2 - 140,
                                  height_ / 4 - 60 + 60 + 45,
                                  0xA0A0A0);
   textRenderer()->drawWithShadow("To prevent level corruption, the current game has quit.",
                                  width_ / 2 - 140,
                                  height_ / 4 - 60 + 60 + 63,
                                  0xA0A0A0);
   textRenderer()->drawWithShadow(
       "Please restart the game.", width_ / 2 - 140, height_ / 4 - 60 + 60 + 81, 0xA0A0A0);
  }
  Screen::render(mouseX, mouseY, tickDelta);
 }
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kOutOfMemory;
 }

 private:
 int ticksRan_ = 0;
};
} // namespace net::minecraft::client::gui::screen
