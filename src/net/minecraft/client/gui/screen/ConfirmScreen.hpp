#pragma once

#include "net/minecraft/client/gui/screen/Screen.hpp"

#include <functional>
#include <string>

namespace net::minecraft::client::gui::screen {

class ConfirmScreen : public Screen {
public:
    using ConfirmHandler = std::function<void(bool confirmed)>;

    ConfirmScreen(ScreenFactory returnFactory,
        ConfirmHandler handler,
        std::string message1,
        std::string message2,
        std::string confirmText,
        std::string cancelText);

    void init() override;
    void render(int mouseX, int mouseY, float tickDelta) override;

private:
    ScreenFactory returnFactory_;
    ConfirmHandler handler_;
    std::string message1_;
    std::string message2_;
    std::string confirmText_;
    std::string cancelText_;
};

} // namespace net::minecraft::client::gui::screen
