#pragma once

#include "net/minecraft/client/render/pipeline/WorldRenderContext.hpp"

namespace net::minecraft::client::render::pipeline::passes {

// Java GameRenderer.renderFrame ~461-464: low view-distance sky.
struct SkyPass {
    static void run(WorldRenderContext& ctx);
};

} // namespace net::minecraft::client::render::pipeline::passes
