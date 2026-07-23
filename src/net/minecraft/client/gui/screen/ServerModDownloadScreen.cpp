#include "net/minecraft/client/gui/screen/ServerModDownloadScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/DisconnectedScreen.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
namespace net::minecraft::client::gui::screen {
ServerModDownloadScreen::ServerModDownloadScreen(multiplayer::ClientNetworkHandler* handler,
                                                 std::vector<std::string> missingMods)
    : handler_(handler), missingMods_(std::move(missingMods)) {
}
void ServerModDownloadScreen::init() {
 buttons_.clear();
 addActionButton(layout::listFooterLeftX(width()),
                 layout::dialogFooterY(height()),
                 layout::kConfirmButtonWidth,
                 layout::kDefaultButtonHeight,
                 "Accept",
                 [this] { accept(); });
 addActionButton(layout::listFooterRightX(width()),
                 layout::dialogFooterY(height()),
                 layout::kConfirmButtonWidth,
                 layout::kDefaultButtonHeight,
                 resource::language::I18n::getTranslation("gui.cancel"),
                 [this] { cancel(); });
}
void ServerModDownloadScreen::render(int mouseX, int mouseY, float tickDelta) {
 renderBackground();
 if(textRenderer() != nullptr) {
  drawCenteredTextWithShadow(*textRenderer(), "Server mods required", width() / 2, height() / 2 - 70, 0xFFFFFF);
  drawCenteredTextWithShadow(*textRenderer(),
                             "This native C++ server can send missing mods for download.",
                             width() / 2,
                             height() / 2 - 52,
                             0xD0D0D0);
  int y = height() / 2 - 28;
  for(const std::string& modId : missingMods_) {
   drawCenteredTextWithShadow(*textRenderer(), modId, width() / 2, y, 0xFFFFFF);
   y += 12;
  }
  if(status_.empty()) {
   drawCenteredTextWithShadow(*textRenderer(),
                              "Accept to download into AppData and install before login.",
                              width() / 2,
                              height() / 2 + 32,
                              0xA0A0A0);
  } else {
   drawCenteredTextWithShadow(*textRenderer(), status_, width() / 2, height() / 2 + 32, 0xFFD080);
  }
 }
 Screen::render(mouseX, mouseY, tickDelta);
}
void ServerModDownloadScreen::keyPressed(char character, int keyCode) {
 (void)character;
 if(busy_) {
  return;
 }
 if(escapePressed(keyCode)) {
  cancel();
 }
}
void ServerModDownloadScreen::accept() {
 if(busy_ || handler_ == nullptr || minecraft() == nullptr) {
  return;
 }
 busy_ = true;
 status_ = "Downloading...";
 std::string error;
 if(!handler_->downloadPendingMods(error)) {
  minecraft()->setScreen(std::make_unique<DisconnectedScreen>(
      "connect.failed", "disconnect.genericReason", std::vector<std::string>{error}));
  return;
 }
 status_ = "Authorizing...";
 handler_->continuePendingLogin();
}
void ServerModDownloadScreen::cancel() {
 if(handler_ != nullptr) {
  handler_->cancelPendingModPrompt();
 }
 quitToTitle();
}
} // namespace net::minecraft::client::gui::screen
