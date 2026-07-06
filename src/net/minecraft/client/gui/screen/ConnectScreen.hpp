#pragma once
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerConnector.hpp"
#include <memory>
#include <string>
#include <utility>
namespace net::minecraft::client::gui::screen {
class ConnectScreen : public Screen {
public:
  ConnectScreen(Minecraft* minecraft, std::string host, int port,
                multiplayer::ConnectOptions options = {});
  ~ConnectScreen() override;
  void tick() override;
  void init() override;
  void render(int mouseX, int mouseY, float delta) override;

private:
  void cancelConnection();
  multiplayer::MultiplayerConnector connector_;
};
} // namespace net::minecraft::client::gui::screen
