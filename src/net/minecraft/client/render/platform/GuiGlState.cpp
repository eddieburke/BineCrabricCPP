#include "net/minecraft/client/render/platform/GuiGlState.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"

namespace net::minecraft::client::render::platform {

namespace {

constexpr int kGlRescaleNormal = 32826;
// InGameHud.java crosshair: GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR.
constexpr int kCrosshairBlendSrc = 775;
constexpr int kCrosshairBlendDst = 769;
// InGameHud.java vignette: GL_ZERO, GL_ONE_MINUS_SRC_COLOR.
constexpr int kVignetteBlendSrc = 0;
constexpr int kVignetteBlendDst = 769;

} // namespace

void GuiGlState::setupHudProjection(const double width, const double height) noexcept
{
    gl::GL11::glClear(gl::GL11::GL_DEPTH_BUFFER_BIT);
    gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
    gl::GL11::glLoadIdentity();
    gl::GL11::glOrtho(0.0, width, height, 0.0, 1000.0, 3000.0);
    gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
    gl::GL11::glLoadIdentity();
    gl::GL11::glTranslatef(0.0f, 0.0f, -2000.0f);
}

void GuiGlState::setupProfilerProjection(const int displayWidth, const int displayHeight) noexcept
{
    gl::GL11::glClear(gl::GL11::GL_DEPTH_BUFFER_BIT);
    gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
    gl::GL11::glLoadIdentity();
    gl::GL11::glOrtho(0.0, displayWidth, displayHeight, 0.0, 1000.0, 3000.0);
    gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
    gl::GL11::glLoadIdentity();
    gl::GL11::glTranslatef(0.0f, 0.0f, -2000.0f);
}

void GuiGlState::disableWorldEffects() noexcept
{
    gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
    gl::GL11::glDisable(gl::GL11::GL_FOG);
}

void GuiGlState::resetColor() noexcept
{
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void GuiGlState::enableStandardBlend() noexcept
{
    gl::GL11::glEnable(gl::GL11::GL_BLEND);
    gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
}

void GuiGlState::disableBlend() noexcept
{
    gl::GL11::glDisable(gl::GL11::GL_BLEND);
}

void GuiGlState::beginCrosshairBlend() noexcept
{
    gl::GL11::glEnable(gl::GL11::GL_BLEND);
    gl::GL11::glBlendFunc(kCrosshairBlendSrc, kCrosshairBlendDst);
}

void GuiGlState::endCrosshairBlend() noexcept
{
    gl::GL11::glDisable(gl::GL11::GL_BLEND);
}

void GuiGlState::beginFullscreenOverlay() noexcept
{
    gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glDepthMask(false);
    enableStandardBlend();
    resetColor();
    gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
}

void GuiGlState::endFullscreenOverlay() noexcept
{
    gl::GL11::glDepthMask(true);
    gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
    resetColor();
    enableStandardBlend();
}

void GuiGlState::beginLitHotbarItems() noexcept
{
    disableBlend();
    gl::GL11::glEnable(kGlRescaleNormal);
    gl::GL11::glPushMatrix();
    gl::GL11::glRotatef(120.0f, 1.0f, 0.0f, 0.0f);
    Lighting::turnOn();
    gl::GL11::glPopMatrix();
}

void GuiGlState::endLitHotbarItems() noexcept
{
    Lighting::turnOff();
    gl::GL11::glDisable(kGlRescaleNormal);
}

void GuiGlState::beginUnlitText() noexcept
{
    disableWorldEffects();
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
}

void GuiGlState::endUnlitText() noexcept
{
    resetColor();
}

void GuiGlState::beginAlphaText() noexcept
{
    enableStandardBlend();
    gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
}

void GuiGlState::endAlphaText() noexcept
{
    gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
    disableBlend();
}

void GuiGlState::beginSleepFade() noexcept
{
    gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
}

void GuiGlState::endSleepFade() noexcept
{
    gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
    gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
}

void GuiGlState::beginProfilerDraw() noexcept
{
    gl::GL11::glLineWidth(1.0f);
    gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
}

void GuiGlState::endProfilerDraw() noexcept
{
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
}

} // namespace net::minecraft::client::render::platform
