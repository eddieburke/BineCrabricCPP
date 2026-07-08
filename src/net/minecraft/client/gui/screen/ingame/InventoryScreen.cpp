#include "net/minecraft/client/gui/screen/ingame/InventoryScreen.hpp"
#include <cmath>
#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ContainerLayout.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
namespace net::minecraft::client::gui::screen::ingame {
namespace {
constexpr int kSidePanelGap = 4;
} // namespace
void InventoryScreen::init() {
  HandledScreen::init();
  if(minecraft() != nullptr && minecraft()->player != nullptr) {
    minecraft()->player->increaseStat(achievement::Achievements::OPEN_INVENTORY.statId(), 1);
  }
}
std::string_view InventoryScreen::getScreenUiId() const {
  return mod::screen_ids::kInventory;
}
void InventoryScreen::publishSidePanelEvent(mod::ScreenRegionEvent& event) const {
  event.screen = const_cast<InventoryScreen*>(this);
  event.screenId = mod::screen_ids::kInventory;
  event.region = mod::screen_regions::kSidePanel;
  event.x = containerOriginX() + backgroundWidth + kSidePanelGap;
  event.y = containerOriginY();
  const int available = width() - event.x;
  event.width = available > 0 ? available : 0;
  event.height = backgroundHeight;
  if(event.phase == mod::ScreenRegionPhase::Render) {
    const gl::preset::ScreenFogOff fogCaps;
    gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
    render::platform::Lighting::turnOff();
    mod::hooks().publish(event);
    return;
  }
  mod::hooks().publish(event);
}
void InventoryScreen::render(int mouseX, int mouseY, float tickDelta) {
  HandledScreen::render(mouseX, mouseY, tickDelta);
  mouseX_ = static_cast<float>(mouseX);
  mouseY_ = static_cast<float>(mouseY);
  mod::ScreenRegionEvent sidePanel;
  sidePanel.screen = this;
  sidePanel.phase = mod::ScreenRegionPhase::Render;
  sidePanel.mouseX = mouseX;
  sidePanel.mouseY = mouseY;
  publishSidePanelEvent(sidePanel);
}
void InventoryScreen::onMouseEvent() {
  if(minecraft() == nullptr) {
    return;
  }
  input::InputSystem& input = input::InputSystem::instance();
  const int dw = minecraft()->displayWidth;
  const int dh = minecraft()->displayHeight;
  if(dw <= 0 || dh <= 0) {
    return;
  }
  const auto [mouseX, mouseY] =
      util::mapScreenMouse(dw, dh, width(), height(), input.eventMouseX(), input.eventMouseY());
  const int wheel = input.eventMouseWheel();
  if(wheel != 0) {
    mod::ScreenRegionEvent sidePanel;
    sidePanel.screen = this;
    sidePanel.phase = mod::ScreenRegionPhase::MouseScroll;
    sidePanel.mouseX = mouseX;
    sidePanel.mouseY = mouseY;
    sidePanel.scrollDelta = wheel;
    publishSidePanelEvent(sidePanel);
    if(sidePanel.handled) {
      return;
    }
  }
  if(wheel != 0) {
    return;
  }
  const int button = input.eventMouseButton();
  if(button < 0) {
    return;
  }
  if(input.eventMouseButtonDown()) {
    mouseClicked(mouseX, mouseY, button);
  } else {
    mouseReleased(mouseX, mouseY, button);
  }
}
void InventoryScreen::mouseClicked(int mouseX, int mouseY, int button) {
  mod::ScreenRegionEvent sidePanel;
  sidePanel.screen = this;
  sidePanel.phase = mod::ScreenRegionPhase::MouseClick;
  sidePanel.mouseX = mouseX;
  sidePanel.mouseY = mouseY;
  sidePanel.button = button;
  publishSidePanelEvent(sidePanel);
  if(sidePanel.handled) {
    return;
  }
  HandledScreen::mouseClicked(mouseX, mouseY, button);
}
void InventoryScreen::drawForeground() {
  if(textRenderer() != nullptr) {
    textRenderer_->draw("Crafting", 86, 16, 0x404040);
  }
}
void InventoryScreen::drawBackground(float tickDelta) {
  if(minecraft() == nullptr) {
    return;
  }
  drawContainerTexture("/gui/inventory.png", 0, 0, backgroundWidth, backgroundHeight);
  const int originX = containerOriginX();
  const int originY = containerOriginY();
  if(minecraft()->player == nullptr) {
    return;
  }
  const gl::preset::PlayerPreview previewCaps;
  gl::MatrixGuard previewMatrix;
  gl::translatef(static_cast<float>(originX + 51), static_cast<float>(originY + 75), 50.0f);
  constexpr float scale = 30.0f;
  gl::scalef(-scale, scale, scale);
  gl::rotatef(180.0f, 0.0f, 0.0f, 1.0f);
  entity::player::ClientPlayerEntity& player = *minecraft()->player;
  const float savedBodyYaw = player.bodyYaw;
  const float savedYaw = player.yaw;
  const float savedPitch = player.pitch;
  const float deltaX = static_cast<float>(originX + 51) - mouseX_;
  const float deltaY = static_cast<float>(originY + 75 - 50) - mouseY_;
  gl::rotatef(135.0f, 0.0f, 1.0f, 0.0f);
  render::platform::Lighting::turnOn();
  gl::rotatef(-135.0f, 0.0f, 1.0f, 0.0f);
  gl::rotatef(-static_cast<float>(std::atan(static_cast<double>(deltaY) / 40.0)) * 20.0f, 1.0f, 0.0f, 0.0f);
  player.bodyYaw = static_cast<float>(std::atan(static_cast<double>(deltaX) / 40.0)) * 20.0f;
  player.yaw = static_cast<float>(std::atan(static_cast<double>(deltaX) / 40.0)) * 40.0f;
  player.pitch = -static_cast<float>(std::atan(static_cast<double>(deltaY) / 40.0)) * 20.0f;
  player.minBrightness = 1.0f;
  gl::translatef(0.0f, player.standingEyeHeight, 0.0f);
  auto& dispatcher = render::entity::EntityRenderDispatcher::instance();
  dispatcher.init(minecraft()->world, &minecraft()->textureManager, minecraft()->textRenderer.get(), &player,
                  &minecraft()->options, tickDelta);
  dispatcher.yaw_ = 180.0f;
  dispatcher.render(player, 0.0, 0.0, 0.0, 0.0f, 1.0f);
  player.minBrightness = 0.0f;
  player.bodyYaw = savedBodyYaw;
  player.yaw = savedYaw;
  player.pitch = savedPitch;
  render::platform::Lighting::turnOff();
}
} // namespace net::minecraft::client::gui::screen::ingame
