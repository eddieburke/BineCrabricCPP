#pragma once

#include <cstdint>
#include <functional>

namespace net::minecraft::client::render {

class GameRenderer;

using ColorMaskRestoreFn = std::function<void()>;

namespace pipeline {

// Per-eye world render orchestrator (Java GameRenderer.renderFrame body, post-stereo setup).
class WorldRenderPass {
public:
    void execute(GameRenderer& renderer, float tickDelta, std::int64_t timeNs, int eye,
        ColorMaskRestoreFn restoreColorMask = nullptr, bool clearColorBuffer = true);
};

} // namespace pipeline
} // namespace net::minecraft::client::render
