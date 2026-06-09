#include "net/minecraft/client/render/ProgressRenderer.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/ProgressRenderError.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/util/UiScale.hpp"

#ifdef _WIN32
#include "net/minecraft/client/util/DisplayManager.hpp"
#endif

#include <chrono>
#include <thread>

namespace net::minecraft::client::render {

namespace {

constexpr int kColorDepthClearMask = gl::GL11::GL_COLOR_BUFFER_BIT | gl::GL11::GL_DEPTH_BUFFER_BIT;

} // namespace

ProgressRenderer::ProgressRenderer(Minecraft* minecraftIn)
    : minecraft(minecraftIn)
{
}

bool ProgressRenderer::checkRunningOrAbort() const
{
    if (minecraft != nullptr && minecraft->running) {
        return true;
    }
    if (noAbort) {
        return false;
    }
    throw ProgressRenderError();
}

void ProgressRenderer::setupLoadingProjection()
{
    if (minecraft == nullptr) {
        return;
    }

    const util::UiScale scale =
        util::uiScale(minecraft->options, minecraft->displayWidth, minecraft->displayHeight);
    gl::GL11::glClear(gl::GL11::GL_DEPTH_BUFFER_BIT);
    gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
    gl::GL11::glLoadIdentity();
    gl::GL11::glOrtho(0.0, scale.rawWidth, scale.rawHeight, 0.0, 100.0, 300.0);
    gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
    gl::GL11::glLoadIdentity();
    gl::GL11::glTranslatef(0.0f, 0.0f, -200.0f);
}

void ProgressRenderer::progressStart(const std::string& titleIn)
{
    noAbort = false;
    start(titleIn);
}

void ProgressRenderer::progressStartNoAbort(std::string titleIn)
{
    noAbort = true;
    start(titleIn);
}

void ProgressRenderer::start(const std::string& titleIn)
{
    if (!checkRunningOrAbort()) {
        return;
    }
    title = titleIn;
    lastTime = 0;
    setupLoadingProjection();
}

void ProgressRenderer::progressStage(const std::string& stageIn)
{
    if (!checkRunningOrAbort()) {
        return;
    }
    lastTime = 0;
    stage = stageIn;
    progressStagePercentage(-1);
    lastTime = 0;
}

void ProgressRenderer::renderLoadingFrame(int percentage)
{
    if (minecraft == nullptr) {
        return;
    }

    const util::UiScale scale =
        util::uiScale(minecraft->options, minecraft->displayWidth, minecraft->displayHeight);
    const int scaledWidth = scale.scaledWidth;
    const int scaledHeight = scale.scaledHeight;

    gl::GL11::glClear(gl::GL11::GL_DEPTH_BUFFER_BIT);
    gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
    gl::GL11::glLoadIdentity();
    gl::GL11::glOrtho(0.0, scale.rawWidth, scale.rawHeight, 0.0, 100.0, 300.0);
    gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
    gl::GL11::glLoadIdentity();
    gl::GL11::glTranslatef(0.0f, 0.0f, -200.0f);
    gl::GL11::glClear(kColorDepthClearMask);

    Tessellator& tessellator = Tessellator::INSTANCE;
    const int backgroundTexture = minecraft->textureManager.getTextureId("/gui/background.png");
    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, backgroundTexture);

    constexpr float textureScale = 32.0f;
    tessellator.startQuads();
    tessellator.color(0x404040);
    tessellator.vertex(0.0, static_cast<double>(scaledHeight), 0.0, 0.0,
        static_cast<double>(scaledHeight) / textureScale);
    tessellator.vertex(static_cast<double>(scaledWidth), static_cast<double>(scaledHeight), 0.0,
        static_cast<double>(scaledWidth) / textureScale, static_cast<double>(scaledHeight) / textureScale);
    tessellator.vertex(static_cast<double>(scaledWidth), 0.0, 0.0, static_cast<double>(scaledWidth) / textureScale, 0.0);
    tessellator.vertex(0.0, 0.0, 0.0, 0.0, 0.0);
    tessellator.draw();

    if (percentage >= 0) {
        constexpr int barWidth = 100;
        constexpr int barHeight = 2;
        const int barX = scaledWidth / 2 - barWidth / 2;
        const int barY = scaledHeight / 2 + 16;

        gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
        tessellator.startQuads();
        tessellator.color(0x808080);
        tessellator.vertex(static_cast<double>(barX), static_cast<double>(barY), 0.0);
        tessellator.vertex(static_cast<double>(barX), static_cast<double>(barY + barHeight), 0.0);
        tessellator.vertex(static_cast<double>(barX + barWidth), static_cast<double>(barY + barHeight), 0.0);
        tessellator.vertex(static_cast<double>(barX + barWidth), static_cast<double>(barY), 0.0);
        tessellator.color(0x80FF80);
        tessellator.vertex(static_cast<double>(barX), static_cast<double>(barY), 0.0);
        tessellator.vertex(static_cast<double>(barX), static_cast<double>(barY + barHeight), 0.0);
        tessellator.vertex(static_cast<double>(barX + percentage), static_cast<double>(barY + barHeight), 0.0);
        tessellator.vertex(static_cast<double>(barX + percentage), static_cast<double>(barY), 0.0);
        tessellator.draw();
        gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    }

    if (minecraft->textRenderer != nullptr) {
        font::TextRenderer& textRenderer = *minecraft->textRenderer;
        textRenderer.drawWithShadow(title, (scaledWidth - textRenderer.getWidth(title)) / 2, scaledHeight / 2 - 4 - 16,
            0xFFFFFF);
        textRenderer.drawWithShadow(stage, (scaledWidth - textRenderer.getWidth(stage)) / 2, scaledHeight / 2 - 4 + 8,
            0xFFFFFF);
    }

#ifdef _WIN32
    util::DisplayManager::present();
#endif
    std::this_thread::yield();
}

void ProgressRenderer::progressStagePercentage(int percentage)
{
    if (!checkRunningOrAbort()) {
        return;
    }

    const auto current = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch())
                             .count();
    if (current - lastTime < 20) {
        return;
    }
    lastTime = current;
    renderLoadingFrame(percentage);
}

} // namespace net::minecraft::client::render
