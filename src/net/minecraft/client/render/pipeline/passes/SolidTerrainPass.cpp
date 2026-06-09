#include "net/minecraft/client/render/pipeline/passes/SolidTerrainPass.hpp"

#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"

namespace net::minecraft::client::render::pipeline::passes {

void SolidTerrainPass::run(WorldRenderContext& ctx)
{
    if (ctx.atmosphere == nullptr || ctx.worldRenderer == nullptr || ctx.camera == nullptr) {
        return;
    }

    ctx.atmosphere->applyWorldFog(WorldRenderPass::makeAtmosphereContext(ctx));
    platform::GlState::beginSolidTerrainPass(ctx.terrainTextureId);
    ctx.worldRenderer->render(*ctx.camera, 0, static_cast<double>(ctx.tickDelta));
    platform::GlState::endSolidTerrainPass();
}

} // namespace net::minecraft::client::render::pipeline::passes
