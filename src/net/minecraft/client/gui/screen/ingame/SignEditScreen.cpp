#include "net/minecraft/client/gui/screen/ingame/SignEditScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
namespace net::minecraft::client::gui::screen::ingame {
void SignEditScreen::init() {
  buttons_.clear();
  addCenteredActionButton(height() / 4 + 120, "Done", [this] { closeScreen(); });
}
void SignEditScreen::render(int mouseX, int mouseY, float tickDelta) {
  renderBackground();
  if(textRenderer() != nullptr) {
    drawCenteredTextWithShadow(*textRenderer(), "Edit sign message", width() / 2, 40, 0xFFFFFF);
    for(int i = 0; i < 4; ++i) {
      const int color = (i == currentLine) ? 0xFFFF00 : 0xFFFFFF;
      drawCenteredTextWithShadow(
          *textRenderer(), text[static_cast<std::size_t>(i)], width() / 2, 70 + i * 12, color);
    }
  }
  Screen::render(mouseX, mouseY, tickDelta);
}
void SignEditScreen::keyPressed(char character, int keyCode) {
  if(arrowUpPressed(keyCode)) {
    currentLine = (currentLine + 3) & 3;
    return;
  }
  if(arrowDownPressed(keyCode) || submitPressed(keyCode, character)) {
    currentLine = (currentLine + 1) & 3;
    return;
  }
  if(backspacePressed(keyCode)) {
    if(!text[static_cast<std::size_t>(currentLine)].empty()) {
      text[static_cast<std::size_t>(currentLine)].pop_back();
    }
    return;
  }
  constexpr int kMaxSignLineLength = 15;
  if(character >= ' ' && text[static_cast<std::size_t>(currentLine)].size() < kMaxSignLineLength) {
    text[static_cast<std::size_t>(currentLine)].push_back(character);
  }
  closeOnEscape(keyCode);
}
} // namespace net::minecraft::client::gui::screen::ingame
