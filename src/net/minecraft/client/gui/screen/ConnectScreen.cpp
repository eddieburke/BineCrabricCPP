#include "net/minecraft/client/gui/screen/ConnectScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/screen/DisconnectedScreen.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkBridge.hpp"
#include "net/minecraft/client/network/ClientNetworkHandler.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"

#include <utility>
#include <vector>

namespace net::minecraft::client::gui::screen {

ConnectScreen::ConnectScreen(Minecraft* minecraft, std::string host, int port)
    : connector_(minecraft, std::move(host), port)
{
}

ConnectScreen::~ConnectScreen() = default;

void ConnectScreen::tick()
{
    if (minecraft() == nullptr) {
        return;
    }

    const std::string connectError = connector_.poll(*minecraft());
    if (!connectError.empty()) {
        minecraft()->setScreen(std::make_unique<DisconnectedScreen>(
            "connect.failed", "disconnect.genericReason", std::vector<std::string>{connectError}));
        return;
    }

    connector_.tickBridge(*minecraft());
}

void ConnectScreen::render(int mouseX, int mouseY, float delta)
{
    renderBackground();
    if (textRenderer() != nullptr) {
        multiplayer::ClientNetworkBridge* bridge = connector_.activeBridge(minecraft());
        const bool connected = bridge != nullptr;
        const std::string title = connected ? resource::language::I18n::getTranslation("connect.authorizing")
                                            : resource::language::I18n::getTranslation("connect.connecting");
        drawCenteredTextWithShadow(*textRenderer(), title, width_ / 2, height_ / 2 - 50, 0xFFFFFF);
        const std::string message =
            connected && bridge->handler() != nullptr ? bridge->handler()->message : "";
        drawCenteredTextWithShadow(*textRenderer(), message, width_ / 2, height_ / 2 - 10, 0xFFFFFF);
    }
    Screen::render(mouseX, mouseY, delta);
}

} // namespace net::minecraft::client::gui::screen
