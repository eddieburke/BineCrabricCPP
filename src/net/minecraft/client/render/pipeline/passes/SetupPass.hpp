#pragma once

#include "net/minecraft/client/render/pipeline/WorldRenderContext.hpp"

namespace net::minecraft::client::render::pipeline::passes {

// Java GameRenderer.renderFrame ~425-445: defaults, camera, spawn point, targeted entity.
struct SetupPass {
    static void run(WorldRenderContext& ctx);
};

} // namespace net::minecraft::client::render::pipeline::passes
