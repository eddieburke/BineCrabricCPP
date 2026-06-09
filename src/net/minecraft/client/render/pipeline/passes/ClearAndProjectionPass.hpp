#pragma once

#include "net/minecraft/client/render/pipeline/WorldRenderContext.hpp"

namespace net::minecraft::client::render::pipeline::passes {

// Java GameRenderer.renderFrame ~456-460: clear, renderWorld, frustum compute.
struct ClearAndProjectionPass {
    static void run(WorldRenderContext& ctx);
};

} // namespace net::minecraft::client::render::pipeline::passes
