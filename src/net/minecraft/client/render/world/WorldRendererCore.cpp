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
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/render/world/ChunkRenderer.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <memory>
#include <optional>
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
        // Apply a finished background distance sort before this frame's
        // layers render, so layer 0 and layer 1 share one draw order.
        worldRenderer.pollChunkSort();
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
        // Near-lane jobs (block edits next to the player) are few and skip the
        // budget so the edit is visible the frame its mesh finishes.
        if (!job->nearLane && uploadCount >= kMinUploadsPerFrame
            && std::chrono::steady_clock::now() >= uploadDeadline) {
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
        worldRenderer.dirtyChunks_.erase(builder);
        ++uploadCount;
    }
    worldRenderer.pendingMeshUploads_ = std::move(deferredUploads);

    auto enqueueMeshJob = [&](chunk::ChunkBuilder* chunk, int priority) {
        if (chunk == nullptr || chunk->meshJobInFlight || !chunk->dirty) {
            return;
        }
        auto job = chunk::ChunkMeshJob::capture(*chunk, resolvedOpts, fancyGraphics);
        if (job == nullptr) {
            // Region not resident yet; the chunk stays in dirtyChunks_ and is
            // retried once the async loader publishes its neighbors.
            return;
        }
        chunk->meshJobInFlight = true;
        worldRenderer.dirtyChunks_.erase(chunk);
        worldRenderer.meshScheduler_.enqueue(std::move(job), priority);
    };

    if (worldRenderer.world != nullptr && worldRenderer.world->hasPendingLightingUpdates()) {
        worldRenderer.world->doLightingUpdates();
    }

    // Fast lane: chunks dirtied near the camera (block edits) go to the
    // dedicated near worker, bypassing the dirty sorter, the capture budget,
    // and the shared pool's queue. The in-flight cap bounds this frame's
    // snapshot cost; overflow retries next frame and meanwhile stays in
    // dirtyChunks_ as a fallback.
    if (!worldRenderer.nearDirtyChunks_.empty()) {
        constexpr std::size_t kNearLaneMaxInFlight = 8;
        std::vector<chunk::ChunkBuilder*> nearList = std::move(worldRenderer.nearDirtyChunks_);
        worldRenderer.nearDirtyChunks_.clear();
        std::size_t nearInFlight = worldRenderer.meshScheduler_.nearPendingJobs();
        for (chunk::ChunkBuilder* chunk : nearList) {
            if (chunk == nullptr || chunk->meshJobInFlight || !chunk->dirty) {
                continue;
            }
            if (nearInFlight >= kNearLaneMaxInFlight) {
                worldRenderer.nearDirtyChunks_.push_back(chunk);
                continue;
            }
            auto job = chunk::ChunkMeshJob::capture(*chunk, resolvedOpts, fancyGraphics);
            if (job == nullptr) {
                // Region not resident yet; retry once neighbors publish.
                worldRenderer.nearDirtyChunks_.push_back(chunk);
                continue;
            }
            chunk->meshJobInFlight = true;
            worldRenderer.dirtyChunks_.erase(chunk);
            worldRenderer.meshScheduler_.enqueueNear(std::move(job));
            ++nearInFlight;
        }
    }

    // Chunks still waiting on the near lane must not compete with the distant
    // backlog on the shared pool this frame; they retry on the near worker next
    // frame (dirtyChunks_ keeps them alive as a last-resort fallback).
    auto isNearDeferred = [&](const chunk::ChunkBuilder* chunk) -> bool {
        if (chunk == nullptr || worldRenderer.nearDirtyChunks_.empty()) {
            return false;
        }
        auto* mutableChunk = const_cast<chunk::ChunkBuilder*>(chunk);
        return std::find(worldRenderer.nearDirtyChunks_.begin(), worldRenderer.nearDirtyChunks_.end(), mutableChunk)
            != worldRenderer.nearDirtyChunks_.end();
    };

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
        // Region snapshots are the remaining main-thread copy cost; bound
        // them by time the same way uploads are, except under force, which
        // means "compile everything now".
        constexpr std::chrono::milliseconds kCaptureBudget {3};
        const auto captureDeadline = std::chrono::steady_clock::now() + kCaptureBudget;
        constexpr int kMinCapturesPerFrame = 2;
        int captures = 0;
        int priority = 0;

        // Returns false once this frame's in-flight target or capture budget
        // is exhausted.
        auto enqueueBudgeted = [&](chunk::ChunkBuilder* chunk) -> bool {
            if (inFlight >= targetInFlight) {
                return false;
            }
            if (!force && captures >= kMinCapturesPerFrame
                && std::chrono::steady_clock::now() >= captureDeadline) {
                return false;
            }
            if (chunk == nullptr || (force && !chunk->inFrustum) || isNearDeferred(chunk)) {
                return true;
            }
            const bool wasIdle = !chunk->meshJobInFlight && chunk->dirty;
            enqueueMeshJob(chunk, priority++);
            if (wasIdle && chunk->meshJobInFlight) {
                ++inFlight;
                ++captures;
            }
            return true;
        };

        const std::size_t want = targetInFlight - inFlight;
        constexpr std::size_t kSelectionSlack = 16;

        if (worldRenderer.dirtyChunks_.size() <= want + kSelectionSlack) {
            // The whole backlog fits in this frame's budget, so priority order
            // doesn't matter — enqueue directly. This is the steady-state path
            // (block edits, lighting), and it keeps edit remeshes same-frame.
            std::vector<chunk::ChunkBuilder*> dirtySnapshot;
            dirtySnapshot.reserve(worldRenderer.dirtyChunks_.size());
            for (chunk::ChunkBuilder* chunk : worldRenderer.dirtyChunks_) {
                dirtySnapshot.push_back(chunk);
            }
            for (chunk::ChunkBuilder* chunk : dirtySnapshot) {
                if (!enqueueBudgeted(chunk)) {
                    break;
                }
            }
        } else {
            // Big backlog (reload / render-ring move): the background sorter
            // keeps a priority order. Enqueue from the order it finished last
            // frame, then hand it the current dirty set for re-sort. Entries
            // are (key, builder id); ids index chunks_, and the epoch guard
            // discards orders that outlived a reload. A chunk that went clean
            // since the sort is skipped by enqueueMeshJob's dirty check.
            std::optional<world::AsyncChunkSorter::Result> sorted =
                worldRenderer.dirtySorter_.tryTakeResult();
            if (sorted && sorted->epoch == worldRenderer.chunkArrayEpoch_) {
                for (const world::AsyncChunkSorter::Entry& entry : sorted->entries) {
                    if (entry.index < 0
                        || static_cast<std::size_t>(entry.index) >= worldRenderer.chunks_.size()) {
                        continue;
                    }
                    if (!enqueueBudgeted(&worldRenderer.chunks_[static_cast<std::size_t>(entry.index)])) {
                        break;
                    }
                }
            }

            if (worldRenderer.dirtySorter_.canSubmit()) {
                // In-frustum nearest first; out-of-frustum chunks sort behind
                // via a key bias chosen to stay well above any real squared
                // distance while keeping float precision for the distances.
                constexpr float kOutOfFrustumKeyBias = 1.0e8f;
                std::vector<world::AsyncChunkSorter::Entry> entries;
                entries.reserve(worldRenderer.dirtyChunks_.size());
                for (const chunk::ChunkBuilder* chunk : worldRenderer.dirtyChunks_) {
                    if (isNearDeferred(chunk)) {
                        continue;
                    }
                    float key = chunk->squaredDistanceTo(camera);
                    if (!chunk->inFrustum) {
                        key += kOutOfFrustumKeyBias;
                    }
                    entries.push_back({key, chunk->id});
                }
                worldRenderer.dirtySorter_.submit(std::move(entries), worldRenderer.chunkArrayEpoch_);
            }
        }
    }

    return worldRenderer.dirtyChunks_.empty() && worldRenderer.nearDirtyChunks_.empty()
        && worldRenderer.pendingMeshUploads_.empty() && worldRenderer.meshScheduler_.idle();
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
//   2. Group them into chunkRenderers_ by cameraOffset.
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
    std::size_t activeGroups = 0;

    for (chunk::ChunkBuilder* chunk : worldRenderer.chunksInCurrentLayer_) {
        std::size_t slot = activeGroups;
        for (std::size_t k = 0; k < activeGroups; ++k) {
            if (worldRenderer.chunkRenderers_[k].isAt(
                    chunk->cameraOffsetX, chunk->cameraOffsetY, chunk->cameraOffsetZ)) {
                slot = k;
                break;
            }
        }
        if (slot == activeGroups) {
            if (activeGroups == worldRenderer.chunkRenderers_.size()) {
                worldRenderer.chunkRenderers_.emplace_back();
            }
            slot = activeGroups++;
            worldRenderer.chunkRenderers_[slot].init(
                chunk->cameraOffsetX, chunk->cameraOffsetY, chunk->cameraOffsetZ,
                interpX, interpY, interpZ);
        }
        worldRenderer.chunkRenderers_[slot].addGlList(chunk->baseRenderList + layer);
    }

    // Flush — mirrors Java's renderLastChunks call at the end of renderChunks.
    renderLastChunks(worldRenderer, layer, tickDelta);
    return rendered;
}

} // namespace net::minecraft::client::render::internal
