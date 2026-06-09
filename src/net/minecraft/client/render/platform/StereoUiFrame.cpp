#include "net/minecraft/client/render/platform/StereoRendering.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/platform/GuiGlState.hpp"
#include "net/minecraft/client/util/UiScale.hpp"

#ifdef _WIN32
#include "net/minecraft/client/input/InputSystem.hpp"
#endif

namespace net::minecraft::client::render::platform {

namespace {

void restoreFullViewport(int displayWidth, int displayHeight) noexcept
{
    gl::GL11::glViewport(0, 0, displayWidth, displayHeight);
}

void renderUiEye(
    const option::GameOptions& options,
    int displayWidth,
    int displayHeight,
    int eye,
    int halfWidth,
    const StereoUiFrame::EyeRenderFn& renderFn) noexcept
{
    StereoUiEyeContext ctx;
    ctx.eye = eye;
    if (halfWidth > 0) {
        ctx.viewportX = eye * halfWidth;
        ctx.viewportY = 0;
        ctx.viewportWidth = halfWidth;
        ctx.viewportHeight = displayHeight;
    } else {
        ctx.viewportWidth = displayWidth;
        ctx.viewportHeight = displayHeight;
    }

    const util::UiScale scale = util::uiScale(options, ctx.viewportWidth, ctx.viewportHeight);
    ctx.scaledWidth = scale.scaledWidth;
    ctx.scaledHeight = scale.scaledHeight;
    ctx.rawScaledWidth = scale.rawWidth;
    ctx.rawScaledHeight = scale.rawHeight;

#ifdef _WIN32
    input::InputSystem& input = input::InputSystem::instance();
    input.syncCursorFromOs();
    const auto [mouseX, mouseY] = util::mapStereoUiMouse(
        options,
        displayWidth,
        displayHeight,
        ctx.scaledWidth,
        ctx.scaledHeight,
        input.mouseX(),
        input.mouseY());
    ctx.scaledMouseX = mouseX;
    ctx.scaledMouseY = mouseY;
#endif

    gl::GL11::glColorMask(true, true, true, true);
    gl::GL11::glViewport(ctx.viewportX, ctx.viewportY, ctx.viewportWidth, ctx.viewportHeight);
    renderFn(ctx);
}

} // namespace

void StereoUiEyeContext::setupHudProjection() const noexcept
{
    GuiGlState::setupHudProjection(rawScaledWidth, rawScaledHeight);
}

StereoUiFrame::StereoUiFrame(
    const option::GameOptions& options, int displayWidth, int displayHeight) noexcept
    : options_(options)
    , displayWidth_(displayWidth)
    , displayHeight_(displayHeight)
    , sideBySide_(options.isSideBySideActive())
{
}

StereoUiFrame::~StereoUiFrame()
{
    if (sideBySide_) {
        restoreFullViewport(displayWidth_, displayHeight_);
    }
}

void StereoUiFrame::forEachEye(const EyeRenderFn& renderFn) noexcept
{
    if (sideBySide_) {
        const int halfWidth = displayWidth_ / 2;
        for (int eye = 0; eye < 2; ++eye) {
            renderUiEye(options_, displayWidth_, displayHeight_, eye, halfWidth, renderFn);
        }
        return;
    }

    renderUiEye(options_, displayWidth_, displayHeight_, 0, 0, renderFn);
}

} // namespace net::minecraft::client::render::platform
