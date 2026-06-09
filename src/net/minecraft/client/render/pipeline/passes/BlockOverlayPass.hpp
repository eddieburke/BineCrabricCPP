#pragma once

#include "net/minecraft/client/render/pipeline/WorldRenderContext.hpp"

namespace net::minecraft::client::render::pipeline::passes {

// Java GameRenderer.renderFrame ~490-496 and ~527-532: mining progress / block outline.
struct BlockOverlayPass {
    static void runInWater(WorldRenderContext& ctx);
    static void runOnLand(WorldRenderContext& ctx);
};

} // namespace net::minecraft::client::render::pipeline::passes
