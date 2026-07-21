#include "net/minecraft/client/render/ProgressRenderer.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#ifdef _WIN32
#include "net/minecraft/client/util/DisplayManager.hpp"
#endif
#include <chrono>
#include <thread>
namespace net::minecraft::client::render {
namespace {
constexpr int kColorDepthClearMask = gl::attrib::ColorBufferBit | gl::attrib::DepthBufferBit;
} // namespace
ProgressRenderer::ProgressRenderer(Minecraft* minecraftIn) : minecraft(minecraftIn) {
}
bool ProgressRenderer::checkRunningOrAbort() const {
 if(minecraft != nullptr && minecraft->running) {
  return true;
 }
 if(noAbort) {
  return false;
 }
 throw ProgressRenderError();
}
void ProgressRenderer::setupLoadingProjection() {
 if(minecraft == nullptr) {
  return;
 }
 const util::UiScale scale = util::uiScale(minecraft->options, minecraft->displayWidth, minecraft->displayHeight);
 RenderSystem::clear(gl::attrib::DepthBufferBit);
 RenderSystem::matrixMode(gl::matrix_::Projection);
 RenderSystem::loadIdentity();
 RenderSystem::ortho(0.0, scale.rawWidth, scale.rawHeight, 0.0, 100.0, 300.0);
 RenderSystem::matrixMode(gl::matrix_::ModelView);
 RenderSystem::loadIdentity();
 RenderSystem::translate(0.0f, 0.0f, -200.0f);
}
void ProgressRenderer::progressStart(const std::string& titleIn) {
 noAbort = false;
 start(titleIn);
}
void ProgressRenderer::progressStartNoAbort(std::string titleIn) {
 noAbort = true;
 start(titleIn);
}
void ProgressRenderer::start(const std::string& titleIn) {
 if(!checkRunningOrAbort()) {
  return;
 }
 title = titleIn;
 lastTime = 0;
 setupLoadingProjection();
}
void ProgressRenderer::progressStage(const std::string& stageIn) {
 if(!checkRunningOrAbort()) {
  return;
 }
 lastTime = 0;
 stage = stageIn;
 progressStagePercentage(-1);
 lastTime = 0;
}
void ProgressRenderer::renderLoadingFrame(int percentage) {
 if(minecraft == nullptr) {
  return;
 }
 const util::UiScale scale = util::uiScale(minecraft->options, minecraft->displayWidth, minecraft->displayHeight);
 const int scaledWidth = scale.scaledWidth;
 const int scaledHeight = scale.scaledHeight;
 setupLoadingProjection();
 RenderSystem::clear(kColorDepthClearMask);
 Tessellator& tessellator = Tessellator::INSTANCE;
 {
  render::RenderPassScope passScope(render::RenderType::guiTextured());
  RenderSystem::bindTexture(minecraft->textureManager.getTextureId("/gui/background.png"));
  gui::draw::tiledPanel(tessellator, 0, 0, scaledWidth, scaledHeight, 0.0f, 0x404040);
 }
 if(percentage >= 0) {
  constexpr int barWidth = 100;
  constexpr int barHeight = 2;
  const int barX = scaledWidth / 2 - barWidth / 2;
  const int barY = scaledHeight / 2 + 16;
  render::RenderPassScope passScope(render::RenderType::guiTextured());
  tessellator.startQuads();
  gui::draw::appendProgressBar(tessellator, barX, barY, barWidth, barHeight, percentage);
  tessellator.draw();
 }
 if(minecraft->textRenderer != nullptr) {
  font::TextRenderer& textRenderer = *minecraft->textRenderer;
  textRenderer.drawWithShadow(
      title, (scaledWidth - textRenderer.getWidth(title)) / 2, scaledHeight / 2 - 4 - 16, 0xFFFFFF);
  textRenderer.drawWithShadow(
      stage, (scaledWidth - textRenderer.getWidth(stage)) / 2, scaledHeight / 2 - 4 + 8, 0xFFFFFF);
 }
#ifdef _WIN32
 util::DisplayManager::present();
#endif
 std::this_thread::yield();
}
void ProgressRenderer::progressStagePercentage(int percentage) {
 if(!checkRunningOrAbort()) {
  return;
 }
#ifdef _WIN32
 client::diagnostics::pingMainLoopHeartbeat();
#endif
 const auto current =
     std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
         .count();
 if(current - lastTime < 20) {
  return;
 }
 lastTime = current;
 renderLoadingFrame(percentage);
}
} // namespace net::minecraft::client::render
