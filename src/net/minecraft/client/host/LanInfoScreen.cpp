#include "net/minecraft/client/host/LanInfoScreen.hpp"
#include "net/minecraft/client/host/LanHostCoordinator.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/GameMenuScreen.hpp"
namespace net::minecraft::client::host {
void LanInfoScreen::init() {
  buttons_.clear();
  addActionButton(gui::layout::centerBtnX(width()), gui::layout::formCancelBtnY(height()), "Done",
                  [this] { backToGameMenu(); });
}
void LanInfoScreen::backToGameMenu() {
  navigateTo(std::make_unique<client::gui::screen::GameMenuScreen>());
}
void LanInfoScreen::keyPressed(char character, int keyCode) {
  (void)character;
  if(escapePressed(keyCode)) {
    backToGameMenu();
  }
}
void LanInfoScreen::render(int mouseX, int mouseY, float tickDelta) {
  renderBackground();
  if(textRenderer() != nullptr && minecraft() != nullptr) {
    const LanConnectionInfo& info = minecraft()->lanHostCoordinator().connectionInfo();
    drawCenteredTextWithShadow(*textRenderer(), "LAN Info", width() / 2, gui::layout::formTitleY(height()), 0xFFFFFF);
    drawTextWithShadow(*textRenderer(), "Primary LAN address", width() / 2 - 140, height() / 4, 0xA0A0A0);
    drawTextWithShadow(*textRenderer(),
                       info.primaryEndpoint.empty() ? "No RFC1918 IPv4 address found" : info.primaryEndpoint,
                       width() / 2 - 140, height() / 4 + 12, 0xFFFFFF);
    drawTextWithShadow(*textRenderer(), "Loopback", width() / 2 - 140, height() / 4 + 36, 0xA0A0A0);
    drawTextWithShadow(*textRenderer(), info.loopbackEndpoint.empty() ? "127.0.0.1" : info.loopbackEndpoint,
                       width() / 2 - 140, height() / 4 + 48, 0xFFFFFF);
    int lineY = height() / 4 + 72;
    if(info.availableEndpoints.size() > 1) {
      drawTextWithShadow(*textRenderer(), "Other IPv4 addresses", width() / 2 - 140, lineY, 0xA0A0A0);
      lineY += 12;
      for(std::size_t index = 1; index < info.availableEndpoints.size(); ++index) {
        drawTextWithShadow(*textRenderer(), info.availableEndpoints[index], width() / 2 - 140, lineY, 0xFFFFFF);
        lineY += 12;
      }
    }
  }
  Screen::render(mouseX, mouseY, tickDelta);
}
} // namespace net::minecraft::client::host
