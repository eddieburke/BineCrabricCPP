#include "net/minecraft/client/render/pipeline/passes/TranslucentTerrainPass.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"

namespace net::minecraft::client::render::pipeline::passes {

namespace {

constexpr int kSmooth = 0x1D01;
constexpr int kFlat = 0x1D00;
constexpr int kSrcAlpha = 0x0302;
constexpr int kOneMinusSrcAlpha = 0x0303;

} // namespace

void TranslucentTerrainPass::run(WorldRenderContext& ctx)
{
    if (ctx.atmosphere == nullptr || ctx.worldRenderer == nullptr || ctx.camera == nullptr
        || ctx.client == nullptr) {
        return;
    }

    gl::GL11::glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
    ctx.atmosphere->applyWorldFog(WorldRenderPass::makeAtmosphereContext(ctx));
    platform::GlState::beginTranslucentTerrainPass(ctx.terrainTextureId);

    if (ctx.client->options.fancyGraphics) {
        if (ctx.client->options.ao) {
            gl::GL11::glShadeModel(kSmooth);
        }
        gl::GL11::glColorMask(false, false, false, false);
        const int translucentCount = ctx.worldRenderer->render(*ctx.camera, 1, static_cast<double>(ctx.tickDelta));
        if (ctx.restoreColorMask) {
            ctx.restoreColorMask();
        } else {
            gl::GL11::glColorMask(true, true, true, true);
        }
        if (translucentCount > 0) {
            ctx.worldRenderer->renderLastChunks(1, static_cast<double>(ctx.tickDelta));
        }
        gl::GL11::glShadeModel(kFlat);
    } else {
        ctx.worldRenderer->render(*ctx.camera, 1, static_cast<double>(ctx.tickDelta));
    }

    platform::GlState::endTranslucentTerrainPass();
}

} // namespace net::minecraft::client::render::pipeline::passes
