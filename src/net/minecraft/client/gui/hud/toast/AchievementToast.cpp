#include "net/minecraft/client/gui/hud/toast/AchievementToast.hpp"
#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include <chrono>
#include <cmath>
namespace net::minecraft::client::gui::hud::toast {
namespace {
std::int64_t nowMillis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
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
  gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glDepthMask(false);
  gl::GL11::glDisable(gl::GL11::GL_FOG);
  const int textureId = client_->textureManager.getTextureId("/achievement/bg.png");
  const achievement::AchievementDef* achievement = achievement::Achievements::getByStatId(achievementStatId_);
  const util::UiScale scale = util::uiScale(client_->options, client_->displayWidth, client_->displayHeight);
  gl::GL11::glDisable(gl::GL11::GL_CULL_FACE);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  gl::GL11::glClear(gl::GL11::GL_DEPTH_BUFFER_BIT);
  gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
  gl::GL11::glLoadIdentity();
  gl::GL11::glOrtho(0.0, scale.rawWidth, scale.rawHeight, 0.0, 1000.0, 3000.0);
  gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
  gl::GL11::glLoadIdentity();
  gl::GL11::glTranslatef(0.0f, 0.0f, -2000.0f);
  const int x = scale.scaledWidth - 160;
  const int y = 0 - static_cast<int>(slide * 36.0);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, textureId);
  gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
  drawTexture(x, y, 96, 202, 160, 32);
  gl::GL11::glDisable(gl::GL11::GL_FOG);
  gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  if(tutorialMode_) {
    client_->textRenderer->drawSplit(description_, x + 30, y + 7, 120, 0xFFFFFFFF);
  } else {
    client_->textRenderer->draw(title_, x + 30, y + 7, 0xFFFFFF00);
    client_->textRenderer->draw(description_, x + 30, y + 18, 0xFFFFFFFF);
  }
  if(achievement != nullptr) {
    render::item::ItemRenderer itemRenderer;
    gl::GL11::glPushMatrix();
    gl::GL11::glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    render::platform::Lighting::turnOn();
    gl::GL11::glPopMatrix();
    gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
    gl::GL11::glEnable(gl::GL11::GL_RESCALE_NORMAL);
    gl::GL11::glEnable(gl::GL11::GL_COLOR_MATERIAL);
    gl::GL11::glEnable(gl::GL11::GL_LIGHTING);
    itemRenderer.renderGuiItem(*client_->textRenderer, client_->textureManager,
                               achievement::Achievements::iconStack(*achievement), x + 8, y + 8);
    gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
  }
  gl::GL11::glDepthMask(true);
  gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void AchievementToast::tick() {
  renderOverlay();
}
} // namespace net::minecraft::client::gui::hud::toast
