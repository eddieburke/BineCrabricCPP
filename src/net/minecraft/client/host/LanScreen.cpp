#include "net/minecraft/client/host/LanScreen.hpp"
#include "net/minecraft/client/host/LanHostCoordinator.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/ConnectScreen.hpp"
#include "net/minecraft/client/gui/screen/GameMenuScreen.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerConnector.hpp"
namespace net::minecraft::client::host {
LanScreen::LanScreen(std::string errorMessage, std::string portText)
    : errorMessage_(std::move(errorMessage)), portText_(std::move(portText)) {}
void LanScreen::init() {
  enableTextInput();
  buttons_.clear();
  openButton_ = &addActionButton(gui::layout::centerBtnX(width()), gui::layout::formPrimaryBtnY(height()), "Open",
                                 [this] { openLan(); });
  addActionButton(gui::layout::centerBtnX(width()), gui::layout::formCancelBtnY(height()), "Cancel",
                  [this] { backToGameMenu(); });
  if(textRenderer() != nullptr) {
    portField_ = std::make_unique<gui::widget::TextFieldWidget>(
        this, textRenderer(), width() / 2 - 100, height() / 4 + 28, 200, 20, portText_.empty() ? "25565" : portText_);
    portField_->focused = true;
    portField_->setMaxLength(5);
  }
  updateOpenButtonState();
}
void LanScreen::tick() {
  tickTextFields({portField_.get()});
  pollServerStart();
  updateOpenButtonState();
}
void LanScreen::removed() {
  disableTextInput();
}
void LanScreen::updateOpenButtonState() {
  if(openButton_ != nullptr) {
    openButton_->active = !startingServer_ && portField_ != nullptr && !portField_->getText().empty();
  }
}
void LanScreen::pollServerStart() {
  if(!startingServer_ || minecraft() == nullptr) {
    return;
  }
  std::string error;
  if(!minecraft()->lanHostCoordinator().tickHosting(error)) {
    return;
  }
  startingServer_ = false;
  updateOpenButtonState();
  if(minecraft()->lanHostCoordinator().isAwaitingLoopback()) {
    navigateTo(std::make_unique<client::gui::screen::ConnectScreen>(
        minecraft(), "127.0.0.1", minecraft()->lanHostCoordinator().boundPort(),
        multiplayer::ConnectOptions{.lanLoopbackHandoff = true}));
    return;
  }
  errorMessage_ = error.empty() ? minecraft()->lanHostCoordinator().lastError() : std::move(error);
  if(errorMessage_.empty()) {
    errorMessage_ = "Could not open the world to LAN.";
  }
}
void LanScreen::backToGameMenu() {
  if(startingServer_ && minecraft() != nullptr) {
    minecraft()->lanHostCoordinator().onConnectCanceledOrFailed();
    startingServer_ = false;
  }
  navigateTo(std::make_unique<client::gui::screen::GameMenuScreen>());
}
void LanScreen::openLan() {
  if(minecraft() == nullptr || portField_ == nullptr || startingServer_) {
    return;
  }
  int parsedPort = 0;
  try {
    parsedPort = std::stoi(portField_->getText());
  } catch(...) {
    errorMessage_ = "Port must be a number from 1 to 65535.";
    return;
  }
  if(parsedPort < 1 || parsedPort > 65535) {
    errorMessage_ = "Port must be a number from 1 to 65535.";
    return;
  }
  errorMessage_.clear();
  std::string error;
  startingServer_ = true;
  updateOpenButtonState();
  if(!minecraft()->lanHostCoordinator().beginHosting(static_cast<std::uint16_t>(parsedPort), error)) {
    startingServer_ = false;
    updateOpenButtonState();
    errorMessage_ = error.empty() ? "Could not open the world to LAN." : std::move(error);
  }
}
void LanScreen::keyPressed(char character, int keyCode) {
  routeKeyToTextFields(character, keyCode, {portField_.get()});
  if(submitPressed(keyCode, character)) {
    openLan();
    return;
  }
  if(escapePressed(keyCode)) {
    backToGameMenu();
    return;
  }
  updateOpenButtonState();
}
void LanScreen::mouseClicked(int mouseX, int mouseY, int button) {
  Screen::mouseClicked(mouseX, mouseY, button);
  clickTextFields(mouseX, mouseY, button, {portField_.get()});
}
void LanScreen::render(int mouseX, int mouseY, float tickDelta) {
  renderBackground();
  if(textRenderer() != nullptr) {
    drawCenteredTextWithShadow(*textRenderer(), "Open to LAN", width() / 2, gui::layout::formTitleY(height()),
                               0xFFFFFF);
    drawTextWithShadow(*textRenderer(), "Choose a custom port for local network hosting.", width() / 2 - 140,
                       height() / 4 + 2, 0xA0A0A0);
    drawTextWithShadow(*textRenderer(), "Port", width() / 2 - 100, height() / 4 + 16, 0xA0A0A0);
    if(!errorMessage_.empty()) {
      drawCenteredTextWithShadow(*textRenderer(), errorMessage_, width() / 2, gui::layout::formPrimaryBtnY(height()) - 20,
                                 0xFF5555);
    } else if(startingServer_) {
      drawCenteredTextWithShadow(*textRenderer(), "Starting server...", width() / 2,
                                 gui::layout::formPrimaryBtnY(height()) - 20, 0xFFFFFF);
    }
  }
  if(portField_ != nullptr) {
    portField_->render();
  }
  Screen::render(mouseX, mouseY, tickDelta);
}
} // namespace net::minecraft::client::host
