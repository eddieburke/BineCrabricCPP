#include "net/minecraft/client/gui/screen/ConnectScreen.hpp"
#include <utility>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/screen/DisconnectedScreen.hpp"
#include "net/minecraft/client/gui/screen/GameMenuScreen.hpp"
#include "net/minecraft/client/host/ServerProcessCoordinator.hpp"
#include "net/minecraft/client/host/LanScreen.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkBridge.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
namespace net::minecraft::client::gui::screen {
ConnectScreen::ConnectScreen(Minecraft* minecraft, std::string host, int port, multiplayer::ConnectOptions options)
    : connector_(minecraft, std::move(host), port, options) {
}
ConnectScreen::~ConnectScreen() = default;
void ConnectScreen::init() {
 buttons_.clear();
 addCenteredActionButton(layout::dialogFooterY(height_), "Cancel", [this] { cancelConnection(); });
}
void ConnectScreen::cancelConnection() {
 if(minecraft() == nullptr) {
  return;
 }
 const bool serverHandoff = minecraft()->serverProcessCoordinator().isAwaitingLoopback();
 connector_.disconnectActive(*minecraft());
 if(serverHandoff) {
  minecraft()->serverProcessCoordinator().onConnectCanceledOrFailed();
  minecraft()->setScreen(std::make_unique<GameMenuScreen>());
  return;
 }
 if(minecraft()->world != nullptr) {
  closeScreen();
  return;
 }
 quitToTitle();
}
void ConnectScreen::tick() {
 if(minecraft() == nullptr) {
  return;
 }
 const std::string connectError = connector_.poll(*minecraft());
 if(!connectError.empty()) {
  if(minecraft()->serverProcessCoordinator().isAwaitingLoopback()) {
   const std::string portText = std::to_string(minecraft()->serverProcessCoordinator().port());
   minecraft()->serverProcessCoordinator().onConnectCanceledOrFailed(connectError);
   minecraft()->setScreen(std::make_unique<client::host::LanScreen>(connectError, portText));
   return;
  }
  minecraft()->setScreen(std::make_unique<DisconnectedScreen>(
      "connect.failed", "disconnect.genericReason", std::vector<std::string>{connectError}));
  return;
 }
}
void ConnectScreen::render(int mouseX, int mouseY, float delta) {
 renderBackground();
 if(textRenderer() != nullptr) {
  multiplayer::ClientNetworkBridge* bridge = connector_.activeBridge(minecraft());
  const bool connected = bridge != nullptr;
  const std::string title = connected ? resource::language::I18n::getTranslation("connect.authorizing")
                                      : resource::language::I18n::getTranslation("connect.connecting");
  drawCenteredTextWithShadow(*textRenderer(), title, width_ / 2, height_ / 2 - 50, 0xFFFFFF);
  const std::string message = connected && bridge->handler() != nullptr ? bridge->handler()->message : "";
  drawCenteredTextWithShadow(*textRenderer(), message, width_ / 2, height_ / 2 - 10, 0xFFFFFF);
 }
 Screen::render(mouseX, mouseY, delta);
}
} // namespace net::minecraft::client::gui::screen
