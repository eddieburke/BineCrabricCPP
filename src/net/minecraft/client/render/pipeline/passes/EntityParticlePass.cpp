#include "net/minecraft/client/render/pipeline/passes/EntityParticlePass.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"

namespace net::minecraft::client::render::pipeline::passes {

namespace {

constexpr int kBlend = 0x0BE2;
constexpr int kSrcAlpha = 0x0302;
constexpr int kOneMinusSrcAlpha = 0x0303;

} // namespace

void EntityParticlePass::run(WorldRenderContext& ctx)
{
    if (ctx.atmosphere == nullptr || ctx.worldRenderer == nullptr || ctx.camera == nullptr
        || ctx.client == nullptr) {
        return;
    }

    ctx.worldRenderer->renderEntities(ctx.camera->getPosition(ctx.tickDelta), ctx.activeCuller, ctx.tickDelta);
    ctx.client->particleManager.renderLit(ctx.camera, ctx.tickDelta);
    platform::Lighting::turnOff();
    ctx.atmosphere->applyWorldFog(WorldRenderPass::makeAtmosphereContext(ctx));
    gl::GL11::glEnable(kBlend);
    gl::GL11::glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
    ctx.client->particleManager.render(ctx.camera, ctx.tickDelta);
}

} // namespace net::minecraft::client::render::pipeline::passes
