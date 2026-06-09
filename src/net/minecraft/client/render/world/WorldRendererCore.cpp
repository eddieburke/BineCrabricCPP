#include "net/minecraft/client/render/world/WorldRendererCore.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/culling/Culler.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/render/world/ChunkRenderer.hpp"
#include "net/minecraft/client/render/pipeline/PipelineConstants.hpp"
#include "net/minecraft/client/render/world/DirtyChunkSorter.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

namespace net::minecraft::client::render::internal {


void WorldRendererCore::renderLastChunks(WorldRenderer& worldRenderer, int /*layer*/, double /*tickDelta*/)
{
    for (auto& cr : worldRenderer.chunkRenderers_) {
        cr.render();
    }
}

void WorldRendererCore::render(WorldRenderer& worldRenderer, const Entity& camera, int layer, float tickDelta)
{
    auto* living = dynamic_cast<const LivingEntity*>(&camera);
    if (living == nullptr) {
        return;
    }
    render(worldRenderer, const_cast<LivingEntity&>(*living), layer, static_cast<double>(tickDelta));
}

int WorldRendererCore::render(WorldRenderer& worldRenderer, LivingEntity& camera, int layer, double tickDelta)
{
    const std::size_t chunkTotal = worldRenderer.chunks_.size();
    if (chunkTotal == 0) {
        return 0;
    }

    for (int scan = 0; scan < 10; ++scan) {
        worldRenderer.chunkRenderIndex = static_cast<int>((worldRenderer.chunkRenderIndex + 1) % chunkTotal);
        chunk::ChunkBuilder& candidate = worldRenderer.chunks_[static_cast<std::size_t>(worldRenderer.chunkRenderIndex)];
        if (!candidate.dirty || worldRenderer.dirtyChunks_.contains(&candidate)) {
            continue;
        }
        worldRenderer.enqueueDirtyChunk(&candidate);
    }

    // Java WorldRenderer.render: reload on view-distance change, reset per-layer-0 counters,
    // re-sort only when the camera moved. Frustum culling is NOT done here (cullChunks owns it),
    // so layer 0 and layer 1 see an identical visibility set within a frame.
    worldRenderer.reloadIfViewDistanceChanged();

    if (layer == 0) {
        worldRenderer.chunkCount = 0;
        worldRenderer.invisibleChunkCount = 0;
        worldRenderer.compiledChunkCount = 0;
        worldRenderer.emptyChunkCount = 0;
    }

    worldRenderer.sortChunksOnMove(camera);

    worldRenderer.cameraEntity_ = &camera;
    platform::Lighting::turnOff();

    const int sortedCount = static_cast<int>(worldRenderer.sortedChunks_.size());
    return renderChunks(worldRenderer, 0, sortedCount, layer, tickDelta);
}

bool WorldRendererCore::compileChunks(WorldRenderer& worldRenderer, LivingEntity& camera, bool force)
{
    block::BlockRenderManager::fancyGraphics = worldRenderer.activeOptions().fancyGraphics;

    static_assert(pipeline::kDistantRebuildSlots == 2);
    world::DirtyChunkSorter dirtyChunkSorter(camera);
    chunk::ChunkBuilder* distantQueue[pipeline::kDistantRebuildSlots] = {};
    std::vector<chunk::ChunkBuilder*> toRebuild;

    const int initialDirtyCount = static_cast<int>(worldRenderer.dirtyChunks_.size());
    if (worldRenderer.world != nullptr && worldRenderer.world->hasPendingLightingUpdates()) {
        worldRenderer.world->doLightingUpdates();
    }

    int nearCount = 0;
    for (chunk::ChunkBuilder* chunk : worldRenderer.dirtyChunks_) {
        if (chunk == nullptr) {
            continue;
        }
        if (!force) {
            if (chunk->squaredDistanceTo(camera) > pipeline::kNearChunkRebuildDistSq) {
                int n9 = 0;
                for (; n9 < pipeline::kDistantRebuildSlots; ++n9) {
                    if (distantQueue[n9] == nullptr
                        || dirtyChunkSorter.compare(distantQueue[n9], chunk) <= 0) {
                        continue;
                    }
                    break;
                }
                if (--n9 <= 0) {
                    continue;
                }
                int n2 = n9;
                while (--n2 != 0) {
                    distantQueue[n2 - 1] = distantQueue[n2];
                }
                distantQueue[n9] = chunk;
                continue;
            }
        } else if (!chunk->inFrustum) {
            continue;
        }
        // Close chunk: add all (Java rebuilds every close dirty chunk per call).
        ++nearCount;
        toRebuild.push_back(chunk);
    }

    if (!toRebuild.empty()) {
        for (chunk::ChunkBuilder* chunk : toRebuild) {
            worldRenderer.dirtyChunks_.erase(chunk);
        }
        if (toRebuild.size() > 1) {
            std::sort(toRebuild.begin(), toRebuild.end(),
                [&](chunk::ChunkBuilder* a, chunk::ChunkBuilder* b) {
                    return dirtyChunkSorter.compare(a, b) < 0;
                });
        }
        for (int i = static_cast<int>(toRebuild.size()) - 1; i >= 0; --i) {
            chunk::ChunkBuilder* chunk = toRebuild[static_cast<std::size_t>(i)];
            chunk->rebuild();
            chunk->dirty = false;
            chunk->queuedForRebuild = false;
        }
    }

    int distantRebuildCount = 0;
    for (int n = pipeline::kDistantRebuildSlots - 1; n >= 0; --n) {
        chunk::ChunkBuilder* chunk = distantQueue[n];
        if (chunk == nullptr) {
            continue;
        }
        if (!force && !chunk->inFrustum && n != pipeline::kDistantRebuildSlots - 1) {
            distantQueue[0] = nullptr;
            distantQueue[pipeline::kDistantRebuildSlots - 1] = nullptr;
            break;
        }
        worldRenderer.dirtyChunks_.erase(chunk);
        chunk->rebuild();
        chunk->dirty = false;
        chunk->queuedForRebuild = false;
        ++distantRebuildCount;
    }

    return initialDirtyCount == nearCount + distantRebuildCount;
}

void WorldRendererCore::renderEntities(
    WorldRenderer& worldRenderer, const Vec3d& cameraPos, Culler* culler, float tickDelta)
{
    if (worldRenderer.entityRenderCooldown > 0) {
        --worldRenderer.entityRenderCooldown;
        return;
    }
    if (worldRenderer.world == nullptr || worldRenderer.client == nullptr || worldRenderer.cameraEntity_ == nullptr) {
        worldRenderer.entityCount = 0;
        worldRenderer.renderedEntityCount = 0;
        worldRenderer.culledEntityCount = 0;
        return;
    }

    auto* livingCamera = dynamic_cast<LivingEntity*>(worldRenderer.cameraEntity_);

    auto& blockDispatcher = block::entity::BlockEntityRenderDispatcher::instance();
    blockDispatcher.prepare(worldRenderer.world, worldRenderer.textureManager, worldRenderer.client->textRenderer.get(),
        worldRenderer.cameraEntity_, tickDelta);

    auto& entityDispatcher = entity::EntityRenderDispatcher::instance();
    entityDispatcher.init(worldRenderer.world, worldRenderer.textureManager, worldRenderer.client->textRenderer.get(),
        livingCamera, &worldRenderer.activeOptions(), tickDelta);

    worldRenderer.entityCount = 0;
    worldRenderer.renderedEntityCount = 0;
    worldRenderer.culledEntityCount = 0;
    if (livingCamera != nullptr) {
        entity::EntityRenderDispatcher::offsetX =
            livingCamera->lastTickX + (livingCamera->x - livingCamera->lastTickX) * static_cast<double>(tickDelta);
        entity::EntityRenderDispatcher::offsetY =
            livingCamera->lastTickY + (livingCamera->y - livingCamera->lastTickY) * static_cast<double>(tickDelta);
        entity::EntityRenderDispatcher::offsetZ =
            livingCamera->lastTickZ + (livingCamera->z - livingCamera->lastTickZ) * static_cast<double>(tickDelta);
        block::entity::BlockEntityRenderDispatcher::offsetX = entity::EntityRenderDispatcher::offsetX;
        block::entity::BlockEntityRenderDispatcher::offsetY = entity::EntityRenderDispatcher::offsetY;
        block::entity::BlockEntityRenderDispatcher::offsetZ = entity::EntityRenderDispatcher::offsetZ;
    }

    const std::vector<Entity*>& entities = worldRenderer.world->entities();
    worldRenderer.entityCount = static_cast<int>(entities.size()) + static_cast<int>(worldRenderer.world->globalEntities.size());

    const client::option::ResolvedRenderOptions resolved =
        client::option::resolve(worldRenderer.activeOptions());

    for (Entity* entity : worldRenderer.world->globalEntities) {
        if (entity == nullptr) {
            continue;
        }
        ++worldRenderer.renderedEntityCount;
        if (!client::option::shouldRenderEntity(resolved, *entity, cameraPos)) {
            continue;
        }
        entityDispatcher.render(*entity, tickDelta);
    }
    for (Entity* entity : entities) {
        if (entity == nullptr) {
            continue;
        }
        if (!client::option::shouldRenderEntity(resolved, *entity, cameraPos)) {
            continue;
        }
        if (!entity->ignoreFrustumCull && culler != nullptr && !culler->isVisible(entity->boundingBox)) {
            ++worldRenderer.culledEntityCount;
            continue;
        }
        if (entity == worldRenderer.cameraEntity_) {
            auto* playerCamera = dynamic_cast<PlayerEntity*>(worldRenderer.cameraEntity_);
            if (playerCamera != nullptr && !worldRenderer.activeOptions().thirdPerson && !playerCamera->isSleeping()) {
                continue;
            }
        }
        int blockY = MathHelper::floor(entity->y);
        if (blockY < 0) {
            blockY = 0;
        }
        if (blockY >= 128) {
            blockY = 127;
        }
        if (!worldRenderer.world->isPosLoaded(MathHelper::floor(entity->x), blockY, MathHelper::floor(entity->z))) {
            continue;
        }
        ++worldRenderer.renderedEntityCount;
        entityDispatcher.render(*entity, tickDelta);
    }

    for (::net::minecraft::block::entity::BlockEntity* blockEntity : worldRenderer.globalBlockEntities) {
        if (blockEntity != nullptr) {
            blockDispatcher.render(*blockEntity, tickDelta);
        }
    }
}

// Mirrors Java WorldRenderer.renderChunks exactly:
//   1. Collect visible chunks into chunksInCurrentLayer_.
//   2. Group them into chunkRenderers_ by cameraOffset (up to 4 groups).
//   3. Flush via renderLastChunks (one glCallLists per group).
int WorldRendererCore::renderChunks(WorldRenderer& worldRenderer, int from, int to, int layer, double tickDelta)
{
    worldRenderer.chunksInCurrentLayer_.clear();
    int rendered = 0;

    for (int i = from; i < to; ++i) {
        chunk::ChunkBuilder* chunk = worldRenderer.sortedChunks_[static_cast<std::size_t>(i)];
        if (chunk == nullptr) {
            continue;
        }

        if (layer == 0) {
            ++worldRenderer.chunkCount;
            if (chunk->renderLayerEmpty[static_cast<std::size_t>(layer)]) {
                ++worldRenderer.emptyChunkCount;
            } else if (!chunk->inFrustum) {
                ++worldRenderer.invisibleChunkCount;
            } else {
                ++worldRenderer.compiledChunkCount;
            }
        }

        if (chunk->renderLayerEmpty[static_cast<std::size_t>(layer)] || !chunk->inFrustum) {
            continue;
        }

        const int listId = chunk->baseRenderList >= 0 ? chunk->baseRenderList + layer : -1;
        if (listId < 0) {
            continue;
        }

        worldRenderer.chunksInCurrentLayer_.push_back(chunk);
        ++rendered;
    }

    // Compute interpolated camera position for the translation.
    const double interpX = worldRenderer.cameraEntity_ != nullptr
        ? worldRenderer.cameraEntity_->lastTickX
            + (worldRenderer.cameraEntity_->x - worldRenderer.cameraEntity_->lastTickX) * tickDelta
        : 0.0;
    const double interpY = worldRenderer.cameraEntity_ != nullptr
        ? worldRenderer.cameraEntity_->lastTickY
            + (worldRenderer.cameraEntity_->y - worldRenderer.cameraEntity_->lastTickY) * tickDelta
        : 0.0;
    const double interpZ = worldRenderer.cameraEntity_ != nullptr
        ? worldRenderer.cameraEntity_->lastTickZ
            + (worldRenderer.cameraEntity_->z - worldRenderer.cameraEntity_->lastTickZ) * tickDelta
        : 0.0;

    // Reset all ChunkRenderer slots.
    for (auto& cr : worldRenderer.chunkRenderers_) {
        cr.clear();
    }

    // Assign each visible chunk to a ChunkRenderer group by cameraOffset.
    int activeGroups = 0;
    const int maxGroups = static_cast<int>(worldRenderer.chunkRenderers_.size());

    for (chunk::ChunkBuilder* chunk : worldRenderer.chunksInCurrentLayer_) {
        int slot = -1;
        for (int k = 0; k < activeGroups; ++k) {
            if (worldRenderer.chunkRenderers_[static_cast<std::size_t>(k)].isAt(
                    chunk->cameraOffsetX, chunk->cameraOffsetY, chunk->cameraOffsetZ)) {
                slot = k;
                break;
            }
        }
        if (slot < 0 && activeGroups < maxGroups) {
            slot = activeGroups++;
            worldRenderer.chunkRenderers_[static_cast<std::size_t>(slot)].init(
                chunk->cameraOffsetX, chunk->cameraOffsetY, chunk->cameraOffsetZ,
                interpX, interpY, interpZ);
        }
#ifndef NDEBUG
        if (slot < 0) {
            assert(false && "Exceeded camera-offset group budget (Java also drops overflow chunks)");
        }
#endif
        if (slot >= 0) {
            worldRenderer.chunkRenderers_[static_cast<std::size_t>(slot)].addGlList(
                chunk->baseRenderList + layer);
        }
    }

    // Flush — mirrors Java's renderLastChunks call at the end of renderChunks.
    renderLastChunks(worldRenderer, layer, tickDelta);
    return rendered;
}

} // namespace net::minecraft::client::render::internal
