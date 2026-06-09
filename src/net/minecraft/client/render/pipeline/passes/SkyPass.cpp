#include "net/minecraft/client/render/pipeline/passes/SkyPass.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"

namespace net::minecraft::client::render::pipeline::passes {

void SkyPass::run(WorldRenderContext& ctx)
{
    if (ctx.atmosphere == nullptr || ctx.client == nullptr) {
        return;
    }

    if (ctx.client->options.viewDistance < 2
        && client::option::resolve(ctx.client->options).renderSky) {
        const atmosphere::AtmosphereContext atmosphereCtx = WorldRenderPass::makeAtmosphereContext(ctx);
        ctx.atmosphere->applySkyFog(atmosphereCtx);
        ctx.atmosphere->renderSky(atmosphereCtx, ctx.tickDelta);
    }
}

} // namespace net::minecraft::client::render::pipeline::passes
