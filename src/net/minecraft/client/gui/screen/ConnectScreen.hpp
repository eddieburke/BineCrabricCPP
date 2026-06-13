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
    ConnectScreen() = default;
    ConnectScreen(Minecraft* minecraft, std::string host, int port);
    ~ConnectScreen() override;

    void tick() override;

    void init() override
    {
        buttons_.clear();
        addCenteredActionButton(layout::dialogFooterY(height_), "Cancel",
            [this] {
                if (minecraft() == nullptr) {
                    return;
                }
                connector_.disconnectActive(*minecraft());
                quitToTitle();
            });
    }

    void render(int mouseX, int mouseY, float delta) override;

private:
    multiplayer::MultiplayerConnector connector_;
};

} // namespace net::minecraft::client::gui::screen
