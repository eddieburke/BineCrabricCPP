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
#include <chrono>
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
    if (worldRenderer.sections_.empty()) {
        return 0;
    }

    // View-distance reloads and the section frontier are handled in cullChunks
    // (frame phase Idle), so layer 0 and layer 1 share one section set + order.
    if (layer == 0) {
        worldRenderer.chunkCount = 0;
        worldRenderer.invisibleChunkCount = 0;
        worldRenderer.compiledChunkCount = 0;
        worldRenderer.emptyChunkCount = 0;
    }

    worldRenderer.cameraEntity_ = &camera;
    platform::Lighting::turnOff();

    return renderChunks(worldRenderer, layer, tickDelta);
}

bool WorldRendererCore::compileChunks(WorldRenderer& worldRenderer, LivingEntity& camera, bool force)
{
    (void)camera; // distance priority now comes from the draw-ring order

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
        chunk::ChunkBuilder* builder = job->builder;
        if (builder == nullptr) {
            continue;
        }

        if (builder->retired) {
            // Section was evicted while this job was in flight; drop the result.
            // sweepRetiring() recycles its display-list pair below.
            builder->meshJobInFlight = false;
            continue;
        }

        // Near-lane jobs (block edits next to the player) skip the upload budget
        // so the edit is visible the frame its mesh finishes. Distant uploads are
        // time-budgeted: defer the rest to next frame (the job keeps
        // meshJobInFlight = true so it stays owned and is reprocessed).
        if (!job->nearLane && uploadCount >= kMinUploadsPerFrame
            && std::chrono::steady_clock::now() >= uploadDeadline) {
            deferredUploads.push_back(std::move(job));
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
    worldRenderer.sweepRetiring();

    auto enqueueMeshJob = [&](chunk::ChunkBuilder* chunk, int priority) {
        if (chunk == nullptr || chunk->meshJobInFlight || !chunk->dirty) {
            return;
        }
        auto job = chunk::ChunkMeshJob::capture(*chunk, resolvedOpts, fancyGraphics);
        if (job == nullptr) {
            // The 3x3 neighborhood is not fully resident yet; the section stays
            // in dirtyChunks_ and is retried once its neighbors become resident.
            return;
        }
        // Copy the live chunk data into the snapshot HERE, on the main thread.
        // Render pins only stop the chunk being freed; they do not stop the main
        // thread writing blocks/meta/light. Capturing on a worker tears against
        // those writes and yields corrupt/empty meshes.
        if (!job->captureSnapshot()) {
            return;
        }
        chunk->meshJobInFlight = true;
        worldRenderer.dirtyChunks_.erase(chunk);
        worldRenderer.meshScheduler_.enqueue(std::move(job), priority);
    };

    if (worldRenderer.world != nullptr && worldRenderer.world->hasPendingLightingUpdates()) {
        worldRenderer.world->doLightingUpdates();
    }

    // Fast lane: sections the player just edited next to the camera go to the
    // dedicated near worker, bypassing the distant backlog, the per-frame
    // in-flight cap, and the capture/upload budgets, so the edit's remesh lands
    // next frame regardless of render distance.
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
                worldRenderer.nearDirtyChunks_.push_back(chunk); // neighbors not resident yet; retry
                continue;
            }
            if (!job->captureSnapshot()) {
                worldRenderer.nearDirtyChunks_.push_back(chunk);
                continue;
            }
            chunk->meshJobInFlight = true;
            worldRenderer.dirtyChunks_.erase(chunk);
            worldRenderer.meshScheduler_.enqueueNear(std::move(job));
            ++nearInFlight;
        }
    }

    if (!worldRenderer.dirtyChunks_.empty()) {
        // One distance-prioritized, time-budgeted feed. Iterating the near->far
        // draw rings yields nearest-first order, so block edits (which are near
        // the camera) preempt naturally.
        const std::size_t targetInFlight = force
            ? worldRenderer.dirtyChunks_.size()
            : static_cast<std::size_t>(worldRenderer.meshScheduler_.workerCount()) * 3;
        std::size_t inFlight = worldRenderer.meshScheduler_.pendingJobs();

        if (inFlight < targetInFlight) {
            // RegionSnapshot memcpy is the remaining main-thread cost; bound it
            // by time, except under force ("compile everything now").
            constexpr std::chrono::milliseconds kCaptureBudget {3};
            const auto captureDeadline = std::chrono::steady_clock::now() + kCaptureBudget;
            const int minCapturesPerFrame =
                client::option::chunkUpdatesPerPass(resolvedOpts, static_cast<int>(worldRenderer.dirtyChunks_.size()));
            int captures = 0;
            int priority = 0;

            // Returns false once this frame's in-flight target or capture budget
            // is exhausted.
            const auto tryEnqueue = [&](chunk::ChunkBuilder* chunk) -> bool {
                if (inFlight >= targetInFlight) {
                    return false;
                }
                if (!force && captures >= minCapturesPerFrame
                    && std::chrono::steady_clock::now() >= captureDeadline) {
                    return false;
                }
                if (chunk == nullptr || !chunk->dirty || chunk->meshJobInFlight) {
                    return true;
                }
                if (force && !chunk->inFrustum) {
                    return true;
                }
                enqueueMeshJob(chunk, priority++);
                if (chunk->meshJobInFlight) {
                    ++inFlight;
                    ++captures;
                }
                return true;
            };

            bool budgetLeft = true;
            for (const std::vector<chunk::ChunkBuilder*>& ring : worldRenderer.drawRings_) {
                if (!budgetLeft) {
                    break;
                }
                for (chunk::ChunkBuilder* chunk : ring) {
                    if (!tryEnqueue(chunk)) {
                        budgetLeft = false;
                        break;
                    }
                }
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
        if (!client::option::shouldRenderEntity(resolved, *entity, cameraPos)) {
            ++worldRenderer.culledEntityCount;
            continue;
        }
        ++worldRenderer.renderedEntityCount;
        entityDispatcher.render(*entity, tickDelta);
    }
    for (Entity* entity : entities) {
        if (entity == nullptr) {
            continue;
        }
        if (!client::option::shouldRenderEntity(resolved, *entity, cameraPos)) {
            ++worldRenderer.culledEntityCount;
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

// Collect visible sections from the near->far draw rings, group them into
// chunkRenderers_ by cameraOffset, and flush via renderLastChunks (one
// glCallLists per group).
int WorldRendererCore::renderChunks(WorldRenderer& worldRenderer, int layer, double tickDelta)
{
    worldRenderer.chunksInCurrentLayer_.clear();
    int rendered = 0;

    for (const std::vector<chunk::ChunkBuilder*>& ring : worldRenderer.drawRings_) {
        for (chunk::ChunkBuilder* chunk : ring) {
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

            if (chunk->baseRenderList < 0) {
                continue;
            }

            worldRenderer.chunksInCurrentLayer_.push_back(chunk);
            ++rendered;
        }
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
