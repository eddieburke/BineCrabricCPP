#pragma once

#include "net/minecraft/client/render/pipeline/WorldRenderContext.hpp"

namespace net::minecraft::client::render::pipeline::passes {

// Java GameRenderer.renderFrame ~497-523: translucent terrain layer 1.
struct TranslucentTerrainPass {
    static void run(WorldRenderContext& ctx);
};

} // namespace net::minecraft::client::render::pipeline::passes
