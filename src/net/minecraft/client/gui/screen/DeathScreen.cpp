#include "net/minecraft/client/gui/screen/DeathScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"

namespace net::minecraft::client::gui::screen {

void DeathScreen::init()
{
    buttons_.clear();
    addCenteredActionButton(layout::deathScreenBtnY(height(), 0), "Respawn",
        [this] {
            if (minecraft() == nullptr) {
                return;
            }
            if (minecraft()->player != nullptr) {
                minecraft()->player->respawn();
            }
            closeScreen();
        });
    widget::ActionButtonWidget& titleBtn = addCenteredActionButton(
        layout::deathScreenBtnY(height(), 1), "Title menu",
        [this] { quitToTitle(); });
    if (minecraft() != nullptr && minecraft()->session.sessionId.empty()) {
        titleBtn.active = false;
    }
}

void DeathScreen::render(int mouseX, int mouseY, float tickDelta)
{
    fillGradient(0, 0, width_, height_, 0x60500000U, 0xA0600000U);
    if (textRenderer() != nullptr) {
        gl::GL11::glPushMatrix();
        gl::GL11::glScalef(2.0f, 2.0f, 2.0f);
        drawCenteredTextWithShadow(*textRenderer(), "Game over!", width_ / 2 / 2, 30, 0xFFFFFF);
        gl::GL11::glPopMatrix();
        if (minecraft() != nullptr && minecraft()->player != nullptr) {
            drawCenteredTextWithShadow(*textRenderer(),
                "Score: &e" + std::to_string(minecraft()->player->getScore()),
                width_ / 2, 100, 0xFFFFFF);
        }
    }
    Screen::render(mouseX, mouseY, tickDelta);
}

void DeathScreen::keyPressed(char character, int keyCode)
{
    (void)character;
    (void)keyCode;
}
} // namespace net::minecraft::client::gui::screen
