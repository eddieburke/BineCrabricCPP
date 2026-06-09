#pragma once

#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"
#include "net/minecraft/client/render/pipeline/PipelineTypes.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderContext.hpp"

#include <cstdint>

namespace net::minecraft::client::render {

class GameRenderer;

namespace pipeline {

struct WorldRenderContext;

// Per-eye world render orchestrator (Java GameRenderer.renderFrame body, post-stereo setup).
class WorldRenderPass {
public:
    void execute(GameRenderer& renderer, float tickDelta, std::int64_t timeNs, int eye,
        ColorMaskRestoreFn restoreColorMask = nullptr, bool clearColorBuffer = true);

    // Friend accessors for passes that need GameRenderer private methods.
    static void renderWorld(GameRenderer& renderer, float tickDelta, int eye);
    static void renderFirstPersonHand(GameRenderer& renderer, float tickDelta, int eye);

    [[nodiscard]] static bool resolveContext(GameRenderer& renderer, float tickDelta, int eye,
        WorldRenderContext& out);

    [[nodiscard]] static atmosphere::AtmosphereContext makeAtmosphereContext(const WorldRenderContext& ctx);
};

} // namespace pipeline
} // namespace net::minecraft::client::render
