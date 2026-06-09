#include "net/minecraft/client/render/pipeline/passes/SetupPass.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/pipeline/WorldRenderPass.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/LegacyChunkCache.hpp"

namespace net::minecraft::client::render::pipeline::passes {

void SetupPass::run(WorldRenderContext& ctx)
{
    platform::GlState::enableWorldDefaults();

    if (ctx.client->camera == nullptr) {
        ctx.client->camera = ctx.client->player;
    }
    if (ctx.eye == 0) {
        ctx.renderer.updateTargetedEntity(ctx.tickDelta);
    }

    if (!WorldRenderPass::resolveContext(ctx.renderer, ctx.tickDelta, ctx.eye, ctx)) {
        return;
    }

    ctx.worldRenderer->setCamera(ctx.camera);
    ctx.camX = ctx.camera->lastTickX
        + (ctx.camera->x - ctx.camera->lastTickX) * static_cast<double>(ctx.tickDelta);
    ctx.camY = ctx.camera->lastTickY
        + (ctx.camera->y - ctx.camera->lastTickY) * static_cast<double>(ctx.tickDelta);
    ctx.camZ = ctx.camera->lastTickZ
        + (ctx.camera->z - ctx.camera->lastTickZ) * static_cast<double>(ctx.tickDelta);

    if (ctx.eye == 0) {
        ChunkSource* chunkSource = ctx.client->world->getChunkSource();
        if (chunkSource != nullptr) {
            if (auto* legacyCache = dynamic_cast<LegacyChunkCache*>(chunkSource)) {
                legacyCache->setSpawnPoint(chunk_coord(MathHelper::floor(ctx.camX)),
                    chunk_coord(MathHelper::floor(ctx.camZ)));
            }
        }
    }

    ctx.terrainTextureId = ctx.client->textureManager.getTextureId("/terrain.png");
}

} // namespace net::minecraft::client::render::pipeline::passes
