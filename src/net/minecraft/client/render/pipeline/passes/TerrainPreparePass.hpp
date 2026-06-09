#pragma once

#include "net/minecraft/client/render/pipeline/WorldRenderContext.hpp"

namespace net::minecraft::client::render::pipeline::passes {

// Java GameRenderer.renderFrame ~465-477: fog, cullChunks, compileChunks.
struct TerrainPreparePass {
    static void run(WorldRenderContext& ctx);
};

} // namespace net::minecraft::client::render::pipeline::passes
