#include "net/minecraft/client/gui/hud/toast/AchievementToast.hpp"
#include <chrono>
#include <cmath>
#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
namespace net::minecraft::client::gui::hud::toast {
namespace {
using net::minecraft::client::render::RenderSystem;
std::int64_t nowMillis() {
 return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
     .count();
}
class ToastDrawScope {
 public:
 ToastDrawScope() : saved_(RenderSystem::getShadow()) {
  RenderSystem::disableDepthTest();
  RenderSystem::depthMask(false);
  RenderSystem::disableCull();
  RenderSystem::enableTexture();
 }
 ~ToastDrawScope() {
  RenderSystem::setShadow(saved_);
 }
 ToastDrawScope(const ToastDrawScope&) = delete;
 ToastDrawScope& operator=(const ToastDrawScope&) = delete;

 private:
 RenderSystem::StateShadow saved_;
};
class ToastItemIconScope {
 public:
 ToastItemIconScope() : saved_(RenderSystem::getShadow()) {
 }
 ~ToastItemIconScope() {
  RenderSystem::setShadow(saved_);
 }
 ToastItemIconScope(const ToastItemIconScope&) = delete;
 ToastItemIconScope& operator=(const ToastItemIconScope&) = delete;

 private:
 RenderSystem::StateShadow saved_;
};
class MatrixScope {
 public:
 MatrixScope() {
  RenderSystem::pushMatrix();
 }
 ~MatrixScope() {
  RenderSystem::popMatrix();
 }
 MatrixScope(const MatrixScope&) = delete;
 MatrixScope& operator=(const MatrixScope&) = delete;
};
} // namespace
void AchievementToast::set(int achievementStatId) {
 const achievement::AchievementDef* achievement = achievement::Achievements::getByStatId(achievementStatId);
 if(achievement == nullptr) {
  return;
 }
 title_ = resource::language::I18n::getTranslation("achievement.get");
 description_ = achievement::Achievements::getTranslatedTitle(*achievement);
 current_ = description_;
 tutorial_.clear();
 achievementStatId_ = achievementStatId;
 startTime_ = nowMillis();
 tutorialMode_ = false;
 active_ = true;
}
void AchievementToast::setTutorial(int achievementStatId) {
 const achievement::AchievementDef* achievement = achievement::Achievements::getByStatId(achievementStatId);
 if(achievement == nullptr || client_ == nullptr) {
  return;
 }
 title_ = achievement::Achievements::getTranslatedTitle(*achievement);
 description_ = achievement::Achievements::getFormattedDescription(
     *achievement, static_cast<int>(client_->options.inventoryKey.code));
 tutorial_.clear();
 current_.clear();
 achievementStatId_ = achievementStatId;
 startTime_ = nowMillis() - 2500;
 tutorialMode_ = true;
 active_ = true;
}
void AchievementToast::clear() {
 current_.clear();
 tutorial_.clear();
 title_.clear();
 description_.clear();
 startTime_ = 0;
 achievementStatId_ = -1;
 active_ = false;
}
void AchievementToast::renderOverlay() {
 if(client_ == nullptr || client_->textRenderer == nullptr || !active_ || startTime_ == 0) {
  return;
 }
 const double elapsed = static_cast<double>(nowMillis() - startTime_) / 3000.0;
 if(!tutorialMode_ && (elapsed < 0.0 || elapsed > 1.0)) {
  clear();
  return;
 }
 double slide = elapsed * 2.0;
 if(slide > 1.0) {
  slide = 2.0 - slide;
 }
 slide *= 4.0;
 slide = 1.0 - slide;
 if(slide < 0.0) {
  slide = 0.0;
 }
 slide *= slide;
 slide *= slide;
 const ToastDrawScope toastCaps;
 const int textureId = client_->textureManager.getTextureId("/achievement/bg.png");
 const achievement::AchievementDef* achievement = achievement::Achievements::getByStatId(achievementStatId_);
 const util::UiScale scale = util::uiScale(client_->options, client_->displayWidth, client_->displayHeight);
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
 RenderSystem::clear(0x00000100);
 RenderSystem::matrixMode(0x1701);
 RenderSystem::loadIdentity();
 RenderSystem::ortho(0.0, scale.rawWidth, scale.rawHeight, 0.0, 1000.0, 3000.0);
 RenderSystem::matrixMode(0x1700);
 RenderSystem::loadIdentity();
 RenderSystem::translate(0.0f, 0.0f, -2000.0f);
 const int x = scale.scaledWidth - 160;
 const int y = 0 - static_cast<int>(slide * 36.0);
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
 RenderSystem::bindTexture(0x0DE1, textureId);
 drawTexture(x, y, 96, 202, 160, 32);
 if(tutorialMode_) {
  client_->textRenderer->drawSplit(description_, x + 30, y + 7, 120, 0xFFFFFFFF);
 } else {
  client_->textRenderer->draw(title_, x + 30, y + 7, 0xFFFFFF00);
  client_->textRenderer->draw(description_, x + 30, y + 18, 0xFFFFFFFF);
 }
 if(achievement != nullptr) {
  render::item::ItemRenderer itemRenderer;
  {
   MatrixScope itemMatrix;
   RenderSystem::rotate(180.0f, 1.0f, 0.0f, 0.0f);
   render::RenderSystem::enableLighting();
  }
  {
   const ToastItemIconScope itemCaps;
   itemRenderer.renderGuiItem(*client_->textRenderer,
                              client_->textureManager,
                              achievement::Achievements::iconStack(*achievement),
                              x + 8,
                              y + 8);
  }
 }
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void AchievementToast::tick() {
 renderOverlay();
}
} // namespace net::minecraft::client::gui::hud::toast
