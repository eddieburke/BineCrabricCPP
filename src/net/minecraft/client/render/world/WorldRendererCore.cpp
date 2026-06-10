#include "net/minecraft/client/render/world/WorldRendererCore.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/culling/Culler.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshScheduler.hpp"
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
#include <chrono>
#include <cassert>
#include <memory>
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
        if (!candidate.dirty || candidate.meshJobInFlight || worldRenderer.dirtyChunks_.contains(&candidate)) {
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
    const client::option::ResolvedRenderOptions resolvedOpts =
        client::option::resolve(worldRenderer.activeOptions());
    const bool fancyGraphics = worldRenderer.activeOptions().fancyGraphics;

    // Uploads compile GL display lists, so they must stay on this thread; cap
    // them by time, not count, so a fast GPU/driver drains the queue instead
    // of pinning throughput to (cap x fps).
    constexpr std::chrono::milliseconds kUploadBudget {4};
    const auto uploadDeadline = std::chrono::steady_clock::now() + kUploadBudget;
    constexpr int kMinUploadsPerFrame = 4;

    std::vector<std::shared_ptr<chunk::ChunkMeshJob>> completed =
        worldRenderer.meshScheduler_.drainCompleted();
    completed.insert(completed.begin(), worldRenderer.pendingMeshUploads_.begin(),
        worldRenderer.pendingMeshUploads_.end());
    worldRenderer.pendingMeshUploads_.clear();

    int uploadCount = 0;
    std::vector<std::shared_ptr<chunk::ChunkMeshJob>> deferredUploads;
    deferredUploads.reserve(completed.size());

    for (std::shared_ptr<chunk::ChunkMeshJob>& job : completed) {
        if (uploadCount >= kMinUploadsPerFrame && std::chrono::steady_clock::now() >= uploadDeadline) {
            deferredUploads.push_back(std::move(job));
            continue;
        }

        chunk::ChunkBuilder* builder = job->builder;
        if (builder == nullptr) {
            continue;
        }

        builder->meshJobInFlight = false;

        if (job->failed || job->version != builder->version) {
            builder->dirty = true;
            worldRenderer.enqueueDirtyChunk(builder);
            continue;
        }

        builder->uploadMesh(*job);
        builder->dirty = false;
        builder->queuedForRebuild = false;
        worldRenderer.dirtyChunks_.erase(builder);
        ++uploadCount;
    }
    worldRenderer.pendingMeshUploads_ = std::move(deferredUploads);

    auto enqueueMeshJob = [&](chunk::ChunkBuilder* chunk, int priority) {
        if (chunk == nullptr || chunk->meshJobInFlight || !chunk->dirty) {
            return;
        }
        auto job = std::make_shared<chunk::ChunkMeshJob>(*chunk);
        job->opts = resolvedOpts;
        job->fancyGraphics = fancyGraphics;
        chunk->meshJobInFlight = true;
        worldRenderer.dirtyChunks_.erase(chunk);
        worldRenderer.meshScheduler_.enqueue(std::move(job), priority);
    };

    world::DirtyChunkSorter dirtyChunkSorter(camera);

    if (worldRenderer.world != nullptr && worldRenderer.world->hasPendingLightingUpdates()) {
        worldRenderer.world->doLightingUpdates();
    }

    // Keep the worker pool saturated instead of mirroring Java's
    // 2-distant-chunks-per-frame trickle: top the queue up to a few jobs per
    // worker every frame so mesh throughput is decoupled from the frame rate.
    // Snapshot copies happen here (main thread), so the budget also bounds the
    // per-frame snapshot cost.
    const std::size_t targetInFlight = force
        ? worldRenderer.dirtyChunks_.size()
        : static_cast<std::size_t>(worldRenderer.meshScheduler_.workerCount()) * 3;
    std::size_t inFlight = worldRenderer.meshScheduler_.pendingJobs();

    if (inFlight < targetInFlight && !worldRenderer.dirtyChunks_.empty()) {
        std::vector<chunk::ChunkBuilder*> candidates(
            worldRenderer.dirtyChunks_.begin(), worldRenderer.dirtyChunks_.end());
        // Best (in-frustum, nearest) first.
        std::sort(candidates.begin(), candidates.end(),
            [&](chunk::ChunkBuilder* a, chunk::ChunkBuilder* b) {
                return dirtyChunkSorter.compare(a, b) > 0;
            });
        int priority = 0;
        for (chunk::ChunkBuilder* chunk : candidates) {
            if (inFlight >= targetInFlight) {
                break;
            }
            if (chunk == nullptr || (force && !chunk->inFrustum)) {
                continue;
            }
            const bool wasIdle = !chunk->meshJobInFlight && chunk->dirty;
            enqueueMeshJob(chunk, priority++);
            if (wasIdle) {
                ++inFlight;
            }
        }
    }

    return worldRenderer.dirtyChunks_.empty() && worldRenderer.pendingMeshUploads_.empty()
        && worldRenderer.meshScheduler_.idle();
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
