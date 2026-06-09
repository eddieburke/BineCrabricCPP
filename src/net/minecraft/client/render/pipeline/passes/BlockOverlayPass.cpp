#include "net/minecraft/client/render/pipeline/passes/BlockOverlayPass.hpp"

#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderHelpers.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"

namespace net::minecraft::client::render::pipeline::passes {

namespace {

constexpr int kAlphaTest = 0x0BC0;

void renderOverlay(WorldRenderContext& ctx, PlayerEntity* player)
{
    gl::GL11::glDisable(kAlphaTest);
    const ItemStack hand = selectedItemOrEmpty(player);
    ctx.worldRenderer->renderMiningProgress(player, *ctx.client->crosshairTarget, 0, hand, ctx.tickDelta);
    ctx.worldRenderer->renderBlockOutline(player, *ctx.client->crosshairTarget, 0, hand, ctx.tickDelta);
    gl::GL11::glEnable(kAlphaTest);
}

} // namespace

void BlockOverlayPass::runInWater(WorldRenderContext& ctx)
{
    if (ctx.worldRenderer == nullptr || ctx.camera == nullptr || ctx.client == nullptr) {
        return;
    }

    auto* player = dynamic_cast<PlayerEntity*>(ctx.camera);
    if (!ctx.client->crosshairTarget.has_value()
        || !ctx.camera->isInFluid(::net::minecraft::block::material::Material::WATER) || player == nullptr) {
        return;
    }
    renderOverlay(ctx, player);
}

void BlockOverlayPass::runOnLand(WorldRenderContext& ctx)
{
    if (ctx.worldRenderer == nullptr || ctx.camera == nullptr || ctx.client == nullptr) {
        return;
    }

    auto* player = dynamic_cast<PlayerEntity*>(ctx.camera);
    if (ctx.zoom != 1.0 || player == nullptr || !ctx.client->crosshairTarget.has_value()
        || ctx.camera->isInFluid(::net::minecraft::block::material::Material::WATER)) {
        return;
    }
    renderOverlay(ctx, player);
}

} // namespace net::minecraft::client::render::pipeline::passes
