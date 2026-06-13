#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"

#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/culling/FrustumCuller.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/render/ViewDistance.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/GameHooks.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft::client::render::pipeline {

namespace {

constexpr int kCullFace = 0x0B44;
constexpr int kDepthBufferBit = 0x00000100;
constexpr int kClearColorAndDepth = 0x00004000 | 0x00000100;
constexpr int kSmooth = 0x1D01;
constexpr int kFlat = 0x1D00;
constexpr int kAlphaTest = 0x0BC0;
constexpr int kBlend = 0x0BE2;
constexpr int kSrcAlpha = 0x0302;
constexpr int kOneMinusSrcAlpha = 0x0303;

// Mutable per-eye state threaded through the pass functions below.
struct WorldRenderContext {
    GameRenderer& renderer;
    float tickDelta = 0.0f;
    std::int64_t timeNs = 0;
    int eye = 0;
    ColorMaskRestoreFn restoreColorMask {};
    bool clearColorBuffer = true;

    net::minecraft::client::Minecraft* client = nullptr;
    double zoom = 1.0;
    net::minecraft::entity::LivingEntity* camera = nullptr;
    WorldRenderer* worldRenderer = nullptr;
    atmosphere::AtmosphereRenderer* atmosphere = nullptr;
    double camX = 0.0;
    double camY = 0.0;
    double camZ = 0.0;
    int terrainTextureId = 0;
    ViewDistance viewDistance {};

    FrustumCuller frustumCuller {};
    Culler* activeCuller = nullptr;
};

[[nodiscard]] ItemStack selectedItemOrEmpty(PlayerEntity* player)
{
    if (player == nullptr) {
        return {};
    }
    const ItemStack* stack = player->inventory.getSelectedItem();
    return stack != nullptr ? *stack : ItemStack {};
}

[[nodiscard]] bool hasWorldScene(const WorldRenderContext& ctx)
{
    return ctx.client != nullptr && ctx.worldRenderer != nullptr && ctx.camera != nullptr;
}

[[nodiscard]] bool resolveContext(float tickDelta, int eye, WorldRenderContext& out)
{
    if (out.client == nullptr) {
        return false;
    }

    auto* living = dynamic_cast<LivingEntity*>(out.client->camera);
    if (living == nullptr || out.client->world == nullptr || out.client->worldRenderer == nullptr
        || out.client->atmosphereRenderer == nullptr) {
        return false;
    }

    out.tickDelta = tickDelta;
    out.eye = eye;
    out.camera = living;
    out.worldRenderer = out.client->worldRenderer.get();
    out.atmosphere = out.client->atmosphereRenderer.get();
    return true;
}

[[nodiscard]] atmosphere::AtmosphereContext makeAtmosphereContext(const WorldRenderContext& ctx)
{
    return atmosphere::AtmosphereContext {
        .client = ctx.client,
        .world = ctx.client->world,
        .textureManager = &ctx.client->textureManager,
        .camera = ctx.client->camera,
        .livingCamera = ctx.camera,
        .options = ctx.client->options,
        .atmosphereTicks = ctx.atmosphere->atmosphereTicks(),
        .viewDistance = ctx.viewDistance,
    };
}

[[nodiscard]] bool hasAtmosphereScene(const WorldRenderContext& ctx)
{
    return hasWorldScene(ctx) && ctx.atmosphere != nullptr;
}

[[nodiscard]] PlayerEntity* overlayPlayer(const WorldRenderContext& ctx)
{
    return ctx.camera != nullptr ? dynamic_cast<PlayerEntity*>(ctx.camera) : nullptr;
}

void renderBlockOverlay(WorldRenderContext& ctx, PlayerEntity& player)
{
    gl::GL11::glDisable(kAlphaTest);
    const ItemStack hand = selectedItemOrEmpty(&player);
    ctx.worldRenderer->renderMiningProgress(&player, *ctx.client->crosshairTarget, 0, hand, ctx.tickDelta);
    ctx.worldRenderer->renderBlockOutline(&player, *ctx.client->crosshairTarget, 0, hand, ctx.tickDelta);
    gl::GL11::glEnable(kAlphaTest);
}

void runSetup(WorldRenderContext& ctx)
{
    platform::GlState::enableWorldDefaults();

    if (ctx.client->camera == nullptr) {
        ctx.client->camera = ctx.client->player;
    }
    if (ctx.eye == 0) {
        ctx.renderer.updateTargetedEntity(ctx.tickDelta);
    }

    if (!resolveContext(ctx.tickDelta, ctx.eye, ctx)) {
        return;
    }

    ctx.viewDistance = ViewDistance::fromOptions(ctx.client->options);
    ctx.worldRenderer->setCamera(ctx.camera);
    ctx.camX = ctx.camera->lastTickX
        + (ctx.camera->x - ctx.camera->lastTickX) * static_cast<double>(ctx.tickDelta);
    ctx.camY = ctx.camera->lastTickY
        + (ctx.camera->y - ctx.camera->lastTickY) * static_cast<double>(ctx.tickDelta);
    ctx.camZ = ctx.camera->lastTickZ
        + (ctx.camera->z - ctx.camera->lastTickZ) * static_cast<double>(ctx.tickDelta);
    mod::CameraSetupEvent cameraEvent {ctx.camera, ctx.camX, ctx.camY, ctx.camZ, ctx.tickDelta};
    mod::hooks().publish(cameraEvent);
    ctx.camX = cameraEvent.x;
    ctx.camY = cameraEvent.y;
    ctx.camZ = cameraEvent.z;

    if (ctx.eye == 0) {
        ctx.client->world->setChunkCacheCenterFromBlockPos(
            MathHelper::floor(ctx.camX),
            MathHelper::floor(ctx.camZ));
    }

    ctx.terrainTextureId = ctx.client->textureManager.getTextureId("/terrain.png");
}

void runClearAndProjection(WorldRenderContext& ctx)
{
    if (ctx.atmosphere == nullptr) {
        return;
    }

    ctx.atmosphere->updateClearAndFogColors(makeAtmosphereContext(ctx), ctx.tickDelta);
    gl::GL11::glClear(ctx.clearColorBuffer ? kClearColorAndDepth : kDepthBufferBit);
    gl::GL11::glEnable(kCullFace);
}

void runSky(WorldRenderContext& ctx)
{
    if (!hasAtmosphereScene(ctx)) {
        return;
    }

    if (ctx.viewDistance.shouldRenderSky()
        && client::option::resolve(ctx.client->options).renderSky) {
        const atmosphere::AtmosphereContext atmosphereCtx = makeAtmosphereContext(ctx);
        ctx.atmosphere->applySkyFog(atmosphereCtx);
        ctx.atmosphere->renderSky(atmosphereCtx, ctx.tickDelta);
    }
}

void runTerrainPrepare(WorldRenderContext& ctx)
{
    if (!hasAtmosphereScene(ctx)) {
        return;
    }

    const atmosphere::AtmosphereContext atmosphereCtx = makeAtmosphereContext(ctx);
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
    ctx.worldRenderer->compileChunks(*ctx.camera, false);
}

void runSolidTerrain(WorldRenderContext& ctx)
{
    if (!hasAtmosphereScene(ctx)) {
        return;
    }

    ctx.atmosphere->applyWorldFog(makeAtmosphereContext(ctx));
    platform::GlState::beginSolidTerrainPass(ctx.terrainTextureId);
    ctx.worldRenderer->render(*ctx.camera, 0, static_cast<double>(ctx.tickDelta));
    platform::GlState::endSolidTerrainPass();
}

void runEntityParticles(WorldRenderContext& ctx)
{
    if (!hasAtmosphereScene(ctx)) {
        return;
    }

    ctx.worldRenderer->renderEntities(ctx.camera->getPosition(ctx.tickDelta), ctx.activeCuller, ctx.tickDelta);
    ctx.client->particleManager.renderLit(ctx.camera, ctx.tickDelta);
    platform::Lighting::turnOff();
    ctx.atmosphere->applyWorldFog(makeAtmosphereContext(ctx));
    gl::GL11::glEnable(kBlend);
    gl::GL11::glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
    ctx.client->particleManager.render(ctx.camera, ctx.tickDelta);
}

void runWaterBlockOverlay(WorldRenderContext& ctx)
{
    if (!hasWorldScene(ctx)) {
        return;
    }

    PlayerEntity* player = overlayPlayer(ctx);
    if (player == nullptr || !ctx.client->crosshairTarget.has_value()
        || !ctx.camera->isInFluid(::net::minecraft::block::material::Material::WATER)) {
        return;
    }
    renderBlockOverlay(ctx, *player);
}

void runTranslucentTerrain(WorldRenderContext& ctx)
{
    if (!hasAtmosphereScene(ctx)) {
        return;
    }

    gl::GL11::glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
    ctx.atmosphere->applyWorldFog(makeAtmosphereContext(ctx));
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

void runLandBlockOverlay(WorldRenderContext& ctx)
{
    if (!hasWorldScene(ctx)) {
        return;
    }

    PlayerEntity* player = overlayPlayer(ctx);
    if (ctx.zoom != 1.0 || player == nullptr || !ctx.client->crosshairTarget.has_value()
        || ctx.camera->isInFluid(::net::minecraft::block::material::Material::WATER)) {
        return;
    }
    renderBlockOverlay(ctx, *player);
}

void runAtmosphereOverlay(WorldRenderContext& ctx)
{
    if (ctx.atmosphere == nullptr || ctx.client == nullptr) {
        return;
    }

    const atmosphere::AtmosphereContext atmosphereCtx = makeAtmosphereContext(ctx);
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

[[nodiscard]] bool prepareHandRender(WorldRenderContext& ctx)
{
    if (ctx.zoom != 1.0) {
        return false;
    }

    platform::GlState::clearDepthForHand();
    return true;
}

} // namespace

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

    runSetup(ctx);
    if (ctx.camera == nullptr || ctx.worldRenderer == nullptr || ctx.atmosphere == nullptr) {
        return;
    }

    runClearAndProjection(ctx);
    renderer.renderWorld(tickDelta, eye);
    if (ctx.client != nullptr && ctx.client->options.frustumCulling) {
        Frustum::getInstance().compute();
    }
    runSky(ctx);
    runTerrainPrepare(ctx);
    runSolidTerrain(ctx);
    runEntityParticles(ctx);
    runWaterBlockOverlay(ctx);
    runTranslucentTerrain(ctx);
    runLandBlockOverlay(ctx);
    runAtmosphereOverlay(ctx);
    if (prepareHandRender(ctx)) {
        renderer.renderFirstPersonHand(tickDelta, eye);
    }
}

} // namespace net::minecraft::client::render::pipeline
