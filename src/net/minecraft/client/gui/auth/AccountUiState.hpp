#pragma once

#include "net/minecraft/client/gui/screen/Screen.hpp"

#include <memory>
#include <string>

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::gui::auth {

struct AccountUiSnapshot {
    bool showSignOutButton = false;
    bool multiplayerReady = false;
    std::string buttonLabel;
    std::string statusLine;
};

[[nodiscard]] AccountUiSnapshot pollAccountUi(const Minecraft& client);
void signOutAccount(Minecraft& client);
[[nodiscard]] std::unique_ptr<gui::screen::Screen> createLoginScreen(gui::screen::ScreenFactory returnFactory);

} // namespace net::minecraft::client::gui::auth
