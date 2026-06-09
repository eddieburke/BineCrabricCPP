#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderContext.hpp"
#include "net/minecraft/client/render/pipeline/passes/AtmosphereOverlayPass.hpp"
#include "net/minecraft/client/render/pipeline/passes/BlockOverlayPass.hpp"
#include "net/minecraft/client/render/pipeline/passes/ClearAndProjectionPass.hpp"
#include "net/minecraft/client/render/pipeline/passes/EntityParticlePass.hpp"
#include "net/minecraft/client/render/pipeline/passes/HandPass.hpp"
#include "net/minecraft/client/render/pipeline/passes/SetupPass.hpp"
#include "net/minecraft/client/render/pipeline/passes/SkyPass.hpp"
#include "net/minecraft/client/render/pipeline/passes/SolidTerrainPass.hpp"
#include "net/minecraft/client/render/pipeline/passes/TerrainPreparePass.hpp"
#include "net/minecraft/client/render/pipeline/passes/TranslucentTerrainPass.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"

namespace net::minecraft::client::render::pipeline {

void WorldRenderPass::renderWorld(GameRenderer& renderer, float tickDelta, int eye)
{
    renderer.renderWorld(tickDelta, eye);
}

void WorldRenderPass::renderFirstPersonHand(GameRenderer& renderer, float tickDelta, int eye)
{
    renderer.renderFirstPersonHand(tickDelta, eye);
}

bool WorldRenderPass::resolveContext(GameRenderer& renderer, float tickDelta, int eye, WorldRenderContext& out)
{
    if (renderer.client == nullptr) {
        return false;
    }

    auto* living = dynamic_cast<LivingEntity*>(renderer.client->camera);
    if (living == nullptr || renderer.client->world == nullptr || renderer.client->worldRenderer == nullptr
        || renderer.client->atmosphereRenderer == nullptr) {
        return false;
    }

    out.tickDelta = tickDelta;
    out.eye = eye;
    out.camera = living;
    out.worldRenderer = renderer.client->worldRenderer.get();
    out.atmosphere = renderer.client->atmosphereRenderer.get();
    return true;
}

atmosphere::AtmosphereContext WorldRenderPass::makeAtmosphereContext(const WorldRenderContext& ctx)
{
    const float viewDistanceBlocks = ctx.renderer.viewDistance > 0.0f
        ? ctx.renderer.viewDistance
        : client::option::resolve(ctx.client->options).viewDistanceBlocks;
    return atmosphere::AtmosphereContext {
        .client = ctx.client,
        .world = ctx.client->world,
        .textureManager = &ctx.client->textureManager,
        .camera = ctx.client->camera,
        .livingCamera = ctx.camera,
        .options = ctx.client->options,
        .atmosphereTicks = ctx.atmosphere->atmosphereTicks(),
        .viewDistanceBlocks = viewDistanceBlocks,
    };
}

void WorldRenderPass::execute(GameRenderer& renderer, float tickDelta, std::int64_t timeNs, int eye,
    ColorMaskRestoreFn restoreColorMask, bool clearColorBuffer)
{
    if (renderer.client == nullptr) {
        return;
    }

    WorldRenderContext ctx {
        .renderer = renderer,
        .tickDelta = tickDelta,
        .timeNs = timeNs,
        .eye = eye,
        .restoreColorMask = std::move(restoreColorMask),
        .clearColorBuffer = clearColorBuffer,
        .client = renderer.client,
        .zoom = renderer.zoom,
    };

    passes::SetupPass::run(ctx);
    if (ctx.camera == nullptr || ctx.worldRenderer == nullptr || ctx.atmosphere == nullptr) {
        return;
    }

    passes::ClearAndProjectionPass::run(ctx);
    passes::SkyPass::run(ctx);
    passes::TerrainPreparePass::run(ctx);
    passes::SolidTerrainPass::run(ctx);
    passes::EntityParticlePass::run(ctx);
    passes::BlockOverlayPass::runInWater(ctx);
    passes::TranslucentTerrainPass::run(ctx);
    passes::BlockOverlayPass::runOnLand(ctx);
    passes::AtmosphereOverlayPass::run(ctx);
    passes::HandPass::run(ctx);
}

} // namespace net::minecraft::client::render::pipeline
