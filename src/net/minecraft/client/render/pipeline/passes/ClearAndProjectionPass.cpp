#include "net/minecraft/client/render/pipeline/passes/ClearAndProjectionPass.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"

namespace net::minecraft::client::render::pipeline::passes {

namespace {

constexpr int kCullFace = 0x0B44;
constexpr int kDepthBufferBit = 0x00000100;
constexpr int kClearColorAndDepth = 0x00004000 | 0x00000100;

} // namespace

void ClearAndProjectionPass::run(WorldRenderContext& ctx)
{
    if (ctx.atmosphere == nullptr) {
        return;
    }

    ctx.atmosphere->updateClearAndFogColors(WorldRenderPass::makeAtmosphereContext(ctx), ctx.tickDelta);
    if (ctx.clearColorBuffer) {
        gl::GL11::glClear(kClearColorAndDepth);
    } else {
        gl::GL11::glClear(kDepthBufferBit);
    }
    gl::GL11::glEnable(kCullFace);

    WorldRenderPass::renderWorld(ctx.renderer, ctx.tickDelta, ctx.eye);
    Frustum::getInstance().compute();
}

} // namespace net::minecraft::client::render::pipeline::passes
