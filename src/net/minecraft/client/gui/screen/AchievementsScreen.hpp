#pragma once

#include "net/minecraft/client/gui/screen/Screen.hpp"

namespace net::minecraft::stat {
class PlayerStats;
}

namespace net::minecraft::client::gui::screen {

class AchievementsScreen : public Screen {
public:
    explicit AchievementsScreen(stat::PlayerStats* stats);

    void init() override;
    void tick() override;
    void render(int mouseX, int mouseY, float tickDelta) override;
    void keyPressed(char character, int keyCode) override;
    void keyPressed(int key) override;
    [[nodiscard]] bool shouldPause() const override { return true; }

private:
    void setTitle();
    void renderIcons(int mouseX, int mouseY, float tickDelta);
    void drawHorizontalLine(int x1, int x2, int y, int color);
    void drawVerticalLine(int x, int y1, int y2, int color);

    stat::PlayerStats* stats_ = nullptr;
    int iconWidth_ = 256;
    int iconHeight_ = 202;
    int prevMouseX_ = 0;
    int prevMouseY_ = 0;
    double mouseX_ = 0.0;
    double mouseY_ = 0.0;
    double scaledMouseDx_ = 0.0;
    double scaledMouseDy_ = 0.0;
    double scrollX_ = 0.0;
    double scrollY_ = 0.0;
    int scroll_ = 0;
};

} // namespace net::minecraft::client::gui::screen
