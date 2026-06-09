#include "net/minecraft/client/gui/screen/ingame/InventoryScreen.hpp"

#include <cmath>

#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ContainerLayout.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"

namespace net::minecraft::client::gui::screen::ingame {

void InventoryScreen::init()
{
    HandledScreen::init();
    if (minecraft() != nullptr && minecraft()->player != nullptr) {
        minecraft()->player->increaseStat(achievement::Achievements::OPEN_INVENTORY.statId(), 1);
    }
}

void InventoryScreen::render(int mouseX, int mouseY, float tickDelta)
{
    HandledScreen::render(mouseX, mouseY, tickDelta);
    mouseX_ = static_cast<float>(mouseX);
    mouseY_ = static_cast<float>(mouseY);
}

void InventoryScreen::drawForeground()
{
    if (textRenderer() != nullptr) {
        textRenderer_->draw("Crafting", 86, 16, 0x404040);
    }
}

void InventoryScreen::drawBackground(float tickDelta)
{
    if (minecraft() == nullptr) {
        return;
    }

    drawContainerTexture("/gui/inventory.png", 0, 0, backgroundWidth, backgroundHeight);
    const int originX = containerOriginX();
    const int originY = containerOriginY();

    if (minecraft()->player == nullptr) {
        return;
    }

    gl::GL11::glEnable(32826);
    gl::GL11::glEnable(2903);
    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(static_cast<float>(originX + 51), static_cast<float>(originY + 75), 50.0f);
    constexpr float scale = 30.0f;
    gl::GL11::glScalef(-scale, scale, scale);
    gl::GL11::glRotatef(180.0f, 0.0f, 0.0f, 1.0f);

    entity::player::ClientPlayerEntity& player = *minecraft()->player;
    const float savedBodyYaw = player.bodyYaw;
    const float savedYaw = player.yaw;
    const float savedPitch = player.pitch;

    const float deltaX = static_cast<float>(originX + 51) - mouseX_;
    const float deltaY = static_cast<float>(originY + 75 - 50) - mouseY_;
    gl::GL11::glRotatef(135.0f, 0.0f, 1.0f, 0.0f);
    render::platform::Lighting::turnOn();
    gl::GL11::glRotatef(-135.0f, 0.0f, 1.0f, 0.0f);
    gl::GL11::glRotatef(-static_cast<float>(std::atan(static_cast<double>(deltaY) / 40.0)) * 20.0f, 1.0f, 0.0f, 0.0f);
    player.bodyYaw = static_cast<float>(std::atan(static_cast<double>(deltaX) / 40.0)) * 20.0f;
    player.yaw = static_cast<float>(std::atan(static_cast<double>(deltaX) / 40.0)) * 40.0f;
    player.pitch = -static_cast<float>(std::atan(static_cast<double>(deltaY) / 40.0)) * 20.0f;
    player.minBrightness = 1.0f;
    gl::GL11::glTranslatef(0.0f, player.standingEyeHeight, 0.0f);

    auto& dispatcher = render::entity::EntityRenderDispatcher::instance();
    dispatcher.init(minecraft()->world, &minecraft()->textureManager, minecraft()->textRenderer.get(), &player,
        &minecraft()->options, tickDelta);
    dispatcher.yaw_ = 180.0f;
    dispatcher.render(player, 0.0, 0.0, 0.0, 0.0f, 1.0f);

    player.minBrightness = 0.0f;
    player.bodyYaw = savedBodyYaw;
    player.yaw = savedYaw;
    player.pitch = savedPitch;
    gl::GL11::glPopMatrix();
    render::platform::Lighting::turnOff();
    gl::GL11::glDisable(32826);
}

} // namespace net::minecraft::client::gui::screen::ingame
