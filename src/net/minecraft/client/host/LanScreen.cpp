#include "net/minecraft/client/host/LanScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/ConnectScreen.hpp"
#include "net/minecraft/client/gui/screen/GameMenuScreen.hpp"
#include "net/minecraft/client/host/ServerProcessCoordinator.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerConnector.hpp"
namespace net::minecraft::client::host {
LanScreen::LanScreen(std::string errorMessage, std::string portText)
    : errorMessage_(std::move(errorMessage)), portText_(std::move(portText)) {
}
void LanScreen::init() {
  enableTextInput();
  buttons_.clear();
  const int baseY = height() / 4;
  openButton_ = &addActionButton(gui::layout::centerBtnX(width()), baseY + 128, "Start Server", [this] {
    openLan();
  });
  addActionButton(gui::layout::centerBtnX(width()), baseY + 152, "Cancel", [this] {
    backToGameMenu();
  });
  if(textRenderer() != nullptr) {
    portField_ = std::make_unique<gui::widget::TextFieldWidget>(this,
                                                                textRenderer(),
                                                                width() / 2 - 100,
                                                                baseY + 28,
                                                                200,
                                                                20,
                                                                portText_.empty() ? "25565" : portText_);
    portField_->focused = true;
    portField_->setMaxLength(5);
  }
  const int left = width() / 2 - 100;
  const int right = width() / 2 + 2;
  pvpButton_ = &addActionButton(left, baseY + 54, 98, 20, "", [this] {
    settings_.pvpEnabled = !settings_.pvpEnabled;
    refreshSettingLabels();
  });
  animalsButton_ = &addActionButton(right, baseY + 54, 98, 20, "", [this] {
    settings_.spawnAnimals = !settings_.spawnAnimals;
    refreshSettingLabels();
  });
  netherButton_ = &addActionButton(left, baseY + 78, 98, 20, "", [this] {
    settings_.allowNether = !settings_.allowNether;
    refreshSettingLabels();
  });
  modsButton_ = &addActionButton(right, baseY + 78, 98, 20, "", [this] {
    settings_.modsEnabled = !settings_.modsEnabled;
    refreshSettingLabels();
  });
  refreshSettingLabels();
  updateOpenButtonState();
}
void LanScreen::refreshSettingLabels() {
  const auto label = [](const std::string& name, bool enabled) { return name + ": " + (enabled ? "ON" : "OFF"); };
  if(pvpButton_ != nullptr) pvpButton_->text = label("PvP", settings_.pvpEnabled);
  if(animalsButton_ != nullptr) animalsButton_->text = label("Animals", settings_.spawnAnimals);
  if(netherButton_ != nullptr) netherButton_->text = label("Nether", settings_.allowNether);
  if(modsButton_ != nullptr) modsButton_->text = label("Server mods", settings_.modsEnabled);
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
  if(!minecraft()->serverProcessCoordinator().pollStart(error)) {
    return;
  }
  startingServer_ = false;
  updateOpenButtonState();
  if(minecraft()->serverProcessCoordinator().isAwaitingLoopback()) {
    navigateTo(std::make_unique<client::gui::screen::ConnectScreen>(
        minecraft(),
        "127.0.0.1",
        minecraft()->serverProcessCoordinator().port(),
        multiplayer::ConnectOptions{.bypassAuthentication = true}));
    return;
  }
  errorMessage_ = error.empty() ? minecraft()->serverProcessCoordinator().lastError() : std::move(error);
  if(errorMessage_.empty()) {
    errorMessage_ = "Could not start dedicated server.";
  }
}
void LanScreen::backToGameMenu() {
  if(startingServer_ && minecraft() != nullptr) {
    minecraft()->serverProcessCoordinator().onConnectCanceledOrFailed();
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
  settings_.port = static_cast<std::uint16_t>(parsedPort);
  if(!minecraft()->serverProcessCoordinator().start(settings_, error)) {
    startingServer_ = false;
    updateOpenButtonState();
    errorMessage_ = error.empty() ? "Could not start dedicated server." : std::move(error);
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
    const int baseY = height() / 4;
    drawCenteredTextWithShadow(
        *textRenderer(), "Start Dedicated Server", width() / 2, gui::layout::formTitleY(height()), 0xFFFFFF);
    if(!errorMessage_.empty()) {
      drawCenteredTextWithShadow(*textRenderer(), errorMessage_, width() / 2, baseY + 2, 0xFF5555);
    } else if(startingServer_) {
      drawCenteredTextWithShadow(
          *textRenderer(), "Starting dedicated server...", width() / 2, baseY + 2, 0xFFFFFF);
    } else {
      drawCenteredTextWithShadow(
          *textRenderer(), "Runs this world in minecraft_server.exe", width() / 2, baseY + 2, 0xA0A0A0);
    }
    drawTextWithShadow(*textRenderer(), "Port", width() / 2 - 100, baseY + 16, 0xA0A0A0);
  }
  if(portField_ != nullptr) {
    portField_->render();
  }
  Screen::render(mouseX, mouseY, tickDelta);
}
} // namespace net::minecraft::client::host
