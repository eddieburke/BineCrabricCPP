#include "net/minecraft/client/render/platform/StereoSession.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"
#include "net/minecraft/client/render/platform/StereoCompositor.hpp"
#include "net/minecraft/client/render/platform/StereoRendering.hpp"
#include "net/minecraft/client/gl/GlExtensions.hpp"

namespace net::minecraft::client::render::platform {

namespace {

StereoSession stereoSession;

} // namespace

void StereoSession::ensureSize(int eyeWidth, int eyeHeight)
{
    gl::GlExtensions::ensureLoaded();
    if (eyeWidth <= 0) {
        eyeWidth = 1;
    }
    if (eyeHeight <= 0) {
        eyeHeight = 1;
    }
    if (eyeWidth_ == eyeWidth && eyeHeight_ == eyeHeight && ready()) {
        return;
    }

    eyeWidth_ = eyeWidth;
    eyeHeight_ = eyeHeight;
    eyes_[0].resize(eyeWidth, eyeHeight);
    eyes_[1].resize(eyeWidth, eyeHeight);
}

void StereoSession::bindEye(int eye) noexcept
{
    if (eye < 0 || eye > 1) {
        return;
    }
    eyes_[eye].bind();
}

void StereoSession::unbind() noexcept
{
    Framebuffer::unbind();
}

void StereoSession::destroy() noexcept
{
    eyes_[0].destroy();
    eyes_[1].destroy();
    eyeWidth_ = 0;
    eyeHeight_ = 0;
}

unsigned int StereoSession::eyeTexture(int eye) const noexcept
{
    if (eye < 0 || eye > 1) {
        return 0;
    }
    return eyes_[eye].colorTexture();
}

bool StereoSession::ready() const noexcept
{
    return eyes_[0].valid() && eyes_[1].valid();
}

void renderStereoFrame(GameRenderer& renderer, pipeline::WorldRenderPass& worldRenderPass, float tickDelta,
    std::int64_t timeNs)
{
    if (renderer.client == nullptr) {
        return;
    }

    const auto& options = renderer.client->options;
    const int displayWidth = renderer.client->displayWidth;
    const int displayHeight = renderer.client->displayHeight;

    if (!options.isStereoActive()) {
        gl::GL11::glViewport(0, 0, displayWidth, displayHeight);
        worldRenderPass.execute(renderer, tickDelta, timeNs, 0);
        return;
    }

    if (options.isSideBySideActive()) {
        for (int eye = 0; eye < 2; ++eye) {
            StereoRendering::beginEyePass(options, eye, displayWidth, displayHeight);
            worldRenderPass.execute(renderer, tickDelta, timeNs, eye, nullptr, true);
        }
        StereoRendering::endFrame(options, displayWidth, displayHeight);
        return;
    }

    if (options.isAnaglyphActive()) {
        stereoSession.ensureSize(displayWidth, displayHeight);

        if (stereoSession.ready()) {
            for (int eye = 0; eye < 2; ++eye) {
                stereoSession.bindEye(eye);
                gl::GL11::glViewport(0, 0, displayWidth, displayHeight);
                worldRenderPass.execute(renderer, tickDelta, timeNs, eye, nullptr, true);
            }
            stereoSession.unbind();
            gl::GL11::glViewport(0, 0, displayWidth, displayHeight);
            StereoCompositor::compositeAnaglyph(stereoSession.eyeTexture(0), stereoSession.eyeTexture(1),
                displayWidth, displayHeight);
            GlState::enableWorldDefaults();
            return;
        }

        for (int eye = 0; eye < 2; ++eye) {
            StereoRendering::beginEyePass(options, eye, displayWidth, displayHeight);
            const ColorMaskRestoreFn restoreColorMask = [&options]() {
                StereoRendering::applyTranslucentDepthMask(options);
            };
            worldRenderPass.execute(renderer, tickDelta, timeNs, eye, restoreColorMask, eye == 0);
        }
        StereoRendering::endFrame(options, displayWidth, displayHeight);
    }
}

} // namespace net::minecraft::client::render::platform
