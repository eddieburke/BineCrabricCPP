#include "net/minecraft/client/render/platform/StereoRendering.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"

namespace net::minecraft::client::render::platform {

namespace {

int activeEye = 0;

void restoreFullViewport(int displayWidth, int displayHeight) noexcept
{
    gl::GL11::glViewport(0, 0, displayWidth, displayHeight);
}

void setSideBySideEyeViewport(int eye, int displayWidth, int displayHeight) noexcept
{
    const int halfWidth = displayWidth / 2;
    gl::GL11::glViewport(eye * halfWidth, 0, halfWidth, displayHeight);
}

} // namespace

void StereoRendering::beginEyePass(
    const option::GameOptions& options, int eye, int displayWidth, int displayHeight) noexcept
{
    if (options.isAnaglyphActive()) {
        activeEye = eye;
        const bool leftRed = !options.ofStereoRedBlueOrder;
        if (eye == 0) {
            gl::GL11::glColorMask(leftRed, !leftRed, !leftRed, false);
        } else {
            gl::GL11::glColorMask(!leftRed, leftRed, leftRed, false);
        }
    } else {
        gl::GL11::glColorMask(true, true, true, true);
    }

    if (options.isSideBySideActive()) {
        setSideBySideEyeViewport(eye, displayWidth, displayHeight);
    } else {
        restoreFullViewport(displayWidth, displayHeight);
    }
}

void StereoRendering::endFrame(const option::GameOptions& options, int displayWidth, int displayHeight) noexcept
{
    if (options.isAnaglyphActive()) {
        gl::GL11::glColorMask(true, true, true, false);
    }
    if (options.isSideBySideActive()) {
        restoreFullViewport(displayWidth, displayHeight);
    }
}

bool StereoRendering::needsSecondEye(const option::GameOptions& options) noexcept
{
    return options.isStereoActive();
}

void StereoRendering::applyTranslucentDepthMask(const option::GameOptions& options) noexcept
{
    if (options.isAnaglyphActive()) {
        const bool leftRed = !options.ofStereoRedBlueOrder;
        if (activeEye == 0) {
            gl::GL11::glColorMask(leftRed, !leftRed, !leftRed, true);
        } else {
            gl::GL11::glColorMask(!leftRed, leftRed, leftRed, true);
        }
    } else {
        gl::GL11::glColorMask(true, true, true, true);
    }
}

void StereoRendering::applyProjectionStereoOffset(int eye, const option::GameOptions& options) noexcept
{
    gl::GL11::glTranslatef(static_cast<float>(-(eye * 2 - 1)) * options.ofStereoOffset, 0.0f, 0.0f);
}

void StereoRendering::applyModelViewStereoOffset(int eye, const option::GameOptions& options) noexcept
{
    gl::GL11::glTranslatef(static_cast<float>(eye * 2 - 1) * options.ofStereoSeparation, 0.0f, 0.0f);
}

} // namespace net::minecraft::client::render::platform
