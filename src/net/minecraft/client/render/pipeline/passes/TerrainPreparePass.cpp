#include "net/minecraft/client/render/pipeline/passes/TerrainPreparePass.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/culling/FrustumCuller.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"

namespace net::minecraft::client::render::pipeline::passes {

namespace {

constexpr int kSmooth = 0x1D01;

} // namespace

void TerrainPreparePass::run(WorldRenderContext& ctx)
{
    if (ctx.atmosphere == nullptr || ctx.worldRenderer == nullptr || ctx.camera == nullptr) {
        return;
    }

    const atmosphere::AtmosphereContext atmosphereCtx = WorldRenderPass::makeAtmosphereContext(ctx);
    platform::GlState::setFogEnabled(true);
    ctx.atmosphere->applyWorldFog(atmosphereCtx);

    if (ctx.client->options.ao) {
        gl::GL11::glShadeModel(kSmooth);
    }

    ctx.worldRenderer->beginWorldRenderFrame();

    ctx.activeCuller = nullptr;
    if (ctx.client->options.frustumCulling) {
        ctx.frustumCuller.prepare(ctx.camX, ctx.camY, ctx.camZ);
        ctx.activeCuller = &ctx.frustumCuller;
    }

    ctx.worldRenderer->cullChunks(ctx.activeCuller, ctx.tickDelta);

    if (ctx.eye == 0) {
        ctx.worldRenderer->compileChunks(*ctx.camera, false);
    }
}

} // namespace net::minecraft::client::render::pipeline::passes
