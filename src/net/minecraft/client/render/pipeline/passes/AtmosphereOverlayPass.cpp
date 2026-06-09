#include "net/minecraft/client/render/pipeline/passes/AtmosphereOverlayPass.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"

namespace net::minecraft::client::render::pipeline::passes {

void AtmosphereOverlayPass::run(WorldRenderContext& ctx)
{
    if (ctx.atmosphere == nullptr || ctx.client == nullptr) {
        return;
    }

    const atmosphere::AtmosphereContext atmosphereCtx = WorldRenderPass::makeAtmosphereContext(ctx);
    ctx.atmosphere->renderPrecipitation(atmosphereCtx, ctx.tickDelta);
    platform::GlState::beginCloudPass();
    ctx.atmosphere->applyWorldFog(atmosphereCtx);
    platform::GlState::setFogEnabled(true);
    platform::Lighting::turnOff();
    if (client::option::resolve(ctx.client->options).renderClouds) {
        ctx.atmosphere->renderClouds(atmosphereCtx, ctx.tickDelta);
    }
    platform::GlState::endCloudPass();
    ctx.atmosphere->applyHandFog(atmosphereCtx);
}

} // namespace net::minecraft::client::render::pipeline::passes
