#pragma once

#include "net/minecraft/client/render/platform/Framebuffer.hpp"

#include <cstdint>

namespace net::minecraft::client::render {

class GameRenderer;

namespace pipeline {
class WorldRenderPass;
}

} // namespace net::minecraft::client::render

namespace net::minecraft::client::render::platform {

// Owns per-eye offscreen targets for stereo world rendering.
class StereoSession {
public:
    void ensureSize(int eyeWidth, int eyeHeight);
    void bindEye(int eye) noexcept;
    void unbind() noexcept;
    void destroy() noexcept;

    [[nodiscard]] unsigned int eyeTexture(int eye) const noexcept;
    [[nodiscard]] bool ready() const noexcept;

private:
    Framebuffer eyes_[2];
    int eyeWidth_ = 0;
    int eyeHeight_ = 0;
};

void renderStereoFrame(GameRenderer& renderer, pipeline::WorldRenderPass& worldRenderPass, float tickDelta,
    std::int64_t timeNs);

} // namespace net::minecraft::client::render::platform
