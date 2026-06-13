#pragma once

#include "net/minecraft/client/gui/screen/Screen.hpp"

#include <string>

namespace net::minecraft::client::gui::screen {

class TitleScreen : public Screen {
public:
    TitleScreen();

    void init() override;
    void tick() override;
    void render(int mouseX, int mouseY, float tickDelta) override;
    void keyPressed(char character, int keyCode) override;

private:
    void applyCalendarSplash();
    std::string chooseSplashText() const;

    float ticks_ = 0.0f;
    std::string splashText_ {};
    widget::ActionButtonWidget* multiplayerButton_ = nullptr;
    widget::ActionButtonWidget* accountButton_ = nullptr;
    bool accountSignedIn_ = false;
};

} // namespace net::minecraft::client::gui::screen
