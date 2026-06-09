#pragma once

#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"

namespace net::minecraft::client::gui::screen::option {

class OptionsScreen : public screen::Screen {
public:
    OptionsScreen(screen::ScreenFactory parentFactory, net::minecraft::client::option::GameOptions* options);

    void init() override;
    void render(int mouseX, int mouseY, float tickDelta) override;

protected:
    void buttonClicked(widget::ButtonWidget& button) override;

private:
    screen::ScreenFactory parentFactory_;
    net::minecraft::client::option::GameOptions* options_ = nullptr;
    std::string title_;
};

} // namespace net::minecraft::client::gui::screen::option
