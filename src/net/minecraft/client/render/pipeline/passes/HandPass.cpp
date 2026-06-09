#include "net/minecraft/client/render/pipeline/passes/HandPass.hpp"

#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"

namespace net::minecraft::client::render::pipeline::passes {

void HandPass::run(WorldRenderContext& ctx)
{
    if (ctx.zoom != 1.0) {
        return;
    }

    platform::GlState::clearDepthForHand();
    WorldRenderPass::renderFirstPersonHand(ctx.renderer, ctx.tickDelta, ctx.eye);
}

} // namespace net::minecraft::client::render::pipeline::passes
