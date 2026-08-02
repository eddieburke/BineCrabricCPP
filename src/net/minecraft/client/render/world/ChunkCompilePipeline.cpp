#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/concurrent/FrameBudget.hpp"
namespace net::minecraft::client::render {
namespace {
constexpr int kBaselineRadius = 13;
constexpr int kMeshBias = 4;
} // namespace
void ChunkCompilePipeline::enqueueDirtyChunk(chunk::ChunkBuilder* chunk) {
 if(chunk == nullptr || chunk->meshJobInFlight) {
  return;
 }
 // P-LITGATE: hold the FIRST mesh of a freshly-created column until its column
 // is marked lit (markChunkColumnLit re-enqueues it). Re-meshes of already-built
 // sections are never held — the gate only delays the first build.
 if(!chunk->built && facade_.chunkSections_.columnPendingLit(chunk->x >> 4, chunk->z >> 4)) {
  return;
 }
 noteNearDirty(chunk);
 dirtyChunks_.insert(chunk);
}
void ChunkCompilePipeline::noteNearDirty(chunk::ChunkBuilder* chunk) {
 constexpr float kNearDirtyDistSq = 32.0f * 32.0f;
 constexpr std::size_t kNearDirtyCap = 64;
 if(nearDirtyChunks_.size() >= kNearDirtyCap) {
  return;
 }
 const net::minecraft::Entity* camera =
     facade_.cameraEntity_ != nullptr ? facade_.cameraEntity_ : (facade_.client != nullptr ? facade_.client->camera : nullptr);
 if(camera == nullptr || chunk->squaredDistanceTo(camera->x, camera->y, camera->z) > kNearDirtyDistSq) {
  return;
 }
 nearDirtyChunks_.insert(chunk);
}
void ChunkCompilePipeline::retireOrFreeSection(std::unique_ptr<chunk::ChunkBuilder> section) {
 if(section == nullptr) {
  return;
 }
 dirtyChunks_.erase(section.get());
 nearDirtyChunks_.erase(section.get());
 if(section->meshJobInFlight) {
  section->retired = true;
  retiring_.push_back(std::move(section));
  return;
 }
 section->freeRegionSlots();
}
void ChunkCompilePipeline::sweepRetiring() {
 for(auto it = retiring_.begin(); it != retiring_.end();) {
  if((*it)->meshJobInFlight) {
   ++it;
   continue;
  }
  (*it)->freeRegionSlots();
  it = retiring_.erase(it);
 }
}
bool ChunkCompilePipeline::startMeshJob(chunk::ChunkBuilder* chunk,
                                        bool nearLane,
                                        int priority,
                                        const client::option::RenderSettings& resolvedOpts,
                                        bool fancyGraphics) {
 if(chunk == nullptr || chunk->meshJobInFlight || !chunk->dirty) {
  return false;
 }
 auto job = chunk::ChunkMeshJob::capture(*chunk, resolvedOpts, fancyGraphics);
 if(job == nullptr) {
  return false;
 }
 // Snapshot capture runs on the mesh worker while pins are held.
 chunk->meshJobInFlight = true;
 dirtyChunks_.erase(chunk);
  if(nearLane) {
   meshScheduler_.enqueueNear(std::move(job));
  } else {
   meshScheduler_.enqueue(std::move(job), priority - kMeshBias);
  }
 return true;
}
bool ChunkCompilePipeline::compileChunks(net::minecraft::entity::LivingEntity& /*camera*/, bool force) {
 facade_.chunkSections_.drainBorderRefresh();
 const client::option::RenderSettings& resolvedOpts = facade_.settings_;
 const bool fancyGraphics = facade_.activeOptions().fancyGraphics;
 const float gridAreaScale = static_cast<float>(facade_.chunkSections_.renderRadiusChunks() *
                                                facade_.chunkSections_.renderRadiusChunks()) /
                             static_cast<float>(kBaselineRadius * kBaselineRadius);
 const std::size_t workerCount = meshScheduler_.workerCount();
 const std::size_t backlog = dirtyChunks_.size() + pendingMeshUploads_.size() + meshScheduler_.pendingJobs();
 const bool loadingBacklog = backlog > 512u;
 const int minUploadsPerFrame = loadingBacklog ? std::clamp(static_cast<int>(workerCount * 2u), 4, 16)
                                               : std::clamp(static_cast<int>(std::ceil(2.0f * gridAreaScale)), 1, 6);
 const net::minecraft::util::concurrent::FrameBudget uploadBudget =
     net::minecraft::util::concurrent::FrameBudget::fromSharedMs(loadingBacklog ? 6 : 3, minUploadsPerFrame);
 // Near-camera edits get a dedicated slice (QD-21): they are uploaded before
 // the ring backlog so a block edit next to the player lands the frame its
 // mesh finishes, not when the distant ring's budget allows (HZ-31).
 const net::minecraft::util::concurrent::FrameBudget nearBudget =
     net::minecraft::util::concurrent::FrameBudget::fromSharedMs(2, minUploadsPerFrame);
 int uploadCount = 0;
 int nearUploadCount = 0;
 std::vector<std::shared_ptr<chunk::ChunkMeshJob>> deferredUploads;
 deferredUploads.reserve(pendingMeshUploads_.size() + meshScheduler_.pendingJobs());
 const auto processUpload = [&](std::shared_ptr<chunk::ChunkMeshJob> job, bool nearLane) {
  chunk::ChunkBuilder* builder = job->builder;
  if(builder == nullptr) {
   return;
  }
  if(builder->retired) {
   builder->meshJobInFlight = false;
   return;
  }
  const net::minecraft::util::concurrent::FrameBudget& budget = nearLane ? nearBudget : uploadBudget;
  const int budgetCount = nearLane ? nearUploadCount : uploadCount;
  if(!budget.hasRemaining(budgetCount)) {
   deferredUploads.push_back(std::move(job));
   return;
  }
  builder->meshJobInFlight = false;
  if(job->failed || job->version != builder->version) {
   builder->dirty = true;
   enqueueDirtyChunk(builder);
   return;
  }
  builder->uploadMesh(*job);
  builder->dirty = false;
  dirtyChunks_.erase(builder);
  if(nearLane) {
   ++nearUploadCount;
  } else {
   ++uploadCount;
  }
 };
 std::vector<std::shared_ptr<chunk::ChunkMeshJob>> nearUploads;
 std::vector<std::shared_ptr<chunk::ChunkMeshJob>> ringUploads;
 nearUploads.reserve(pendingMeshUploads_.size());
 ringUploads.reserve(pendingMeshUploads_.size());
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : pendingMeshUploads_) {
  if(job->nearLane) {
   nearUploads.push_back(std::move(job));
  } else {
   ringUploads.push_back(std::move(job));
  }
 }
 pendingMeshUploads_.clear();
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : meshScheduler_.drainCompleted()) {
  if(job->nearLane) {
   nearUploads.push_back(std::move(job));
  } else {
   ringUploads.push_back(std::move(job));
  }
 }
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : nearUploads) {
  processUpload(std::move(job), true);
 }
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : ringUploads) {
  processUpload(std::move(job), false);
 }
 pendingMeshUploads_ = std::move(deferredUploads);
 sweepRetiring();
 const std::size_t targetInFlight = workerCount * (force ? 6u : 3u);
 std::size_t inFlight = meshScheduler_.pendingJobs();
 if(inFlight < targetInFlight && pendingMeshUploads_.size() < targetInFlight * 2u) {
  const int requestedCaptures =
      client::option::chunkUpdatesPerPass(resolvedOpts, static_cast<int>(dirtyChunks_.size()));
  const int minCapturesPerFrame = force ? static_cast<int>(workerCount * 2u)
                                        : std::clamp(requestedCaptures, 1, static_cast<int>(workerCount));
  const net::minecraft::util::concurrent::FrameBudget captureBudget =
      net::minecraft::util::concurrent::FrameBudget::fromSharedMs((force || loadingBacklog) ? 2 : 1,
                                                                   minCapturesPerFrame);
  int captures = 0;
  const auto canCapture = [&] { return inFlight < targetInFlight && captureBudget.hasRemaining(captures); };
  for(auto it = nearDirtyChunks_.begin(); it != nearDirtyChunks_.end() && canCapture();) {
   chunk::ChunkBuilder* section = *it;
   if(section == nullptr || !section->dirty || section->meshJobInFlight) {
    it = nearDirtyChunks_.erase(it);
    continue;
   }
   if(startMeshJob(section, true, 0, resolvedOpts, fancyGraphics)) {
    it = nearDirtyChunks_.erase(it);
    ++inFlight;
    ++captures;
   } else {
    ++it;
   }
  }
  const auto& drawRings = facade_.chunkSections_.drawRings();
  for(std::size_t ring = 0; ring < drawRings.size() && canCapture(); ++ring) {
   for(chunk::ChunkBuilder* section : drawRings[ring]) {
    if(!canCapture()) {
     break;
    }
    if(section == nullptr || !dirtyChunks_.contains(section)) {
     continue;
    }
    if(!section->dirty) {
     dirtyChunks_.erase(section);
     continue;
    }
    if(section->meshJobInFlight || (force && !section->inFrustum)) {
     continue;
    }
    if(startMeshJob(section, false, facade_.chunkSections_.ringOf(*section), resolvedOpts, fancyGraphics)) {
     ++inFlight;
     ++captures;
    }
   }
  }
 }
 return dirtyChunks_.empty() && nearDirtyChunks_.empty() && pendingMeshUploads_.empty() && meshScheduler_.idle();
}
} // namespace net::minecraft::client::render
