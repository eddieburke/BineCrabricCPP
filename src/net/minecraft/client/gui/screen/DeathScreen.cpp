#include "net/minecraft/client/gui/screen/DeathScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
namespace net::minecraft::client::gui::screen {
namespace core = net::minecraft::client::render::core;
void DeathScreen::init() {
 buttons_.clear();
 addCenteredActionButton(layout::deathScreenBtnY(height(), 0), "Respawn", [this] {
  if(minecraft() == nullptr) {
   return;
  }
  if(minecraft()->player != nullptr) {
   minecraft()->player->respawn();
  }
  closeScreen();
 });
 widget::ActionButtonWidget& titleBtn =
     addCenteredActionButton(layout::deathScreenBtnY(height(), 1), "Title menu", [this] { quitToTitle(); });
 if(minecraft() != nullptr && minecraft()->session.sessionId.empty()) {
  titleBtn.active = false;
 }
}
void DeathScreen::render(int mouseX, int mouseY, float tickDelta) {
 {
  const render::RenderPassScope passScope(render::RenderType::gui());
  draw::verticalGradientQuad(render::Tessellator::INSTANCE, 0, 0, width_, height_, 0x500000, 0x60, 0x600000, 0xA0);
 }
 if(textRenderer() != nullptr) {
  {
   const core::ScopedDrawCameraState textGuard;
   net::minecraft::util::math::Matrix4f pose = core::drawPose();
   pose.scale(2.0f, 2.0f, 2.0f);
   core::setDrawPose(pose);
   textRenderer()->drawCenteredWithShadow("Game over!", width_ / 2 / 2, 30, 0xFFFFFF);
  }
  if(minecraft() != nullptr && minecraft()->player != nullptr) {
   textRenderer()->drawCenteredWithShadow("Score: &e" + std::to_string(minecraft()->player->getScore()),
                                          width_ / 2,
                                          100,
                                          0xFFFFFF);
  }
 }
 Screen::render(mouseX, mouseY, tickDelta);
}
void DeathScreen::keyPressed(char character, int keyCode) {
 (void)character;
 (void)keyCode;
}
} // namespace net::minecraft::client::gui::screen
