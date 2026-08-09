#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include <limits>
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
 // The old P-LITGATE held a fresh column's first mesh until the lighting engine
 // called back. That made "does terrain render at all" depend on a callback
 // chain with two release paths and an idle-fallback — if any of them misses,
 // sections sit dirty forever and the world is simply empty, with no error.
 // Not worth it: meshing immediately costs at most one dark first build, and
 // the drained lighting region re-dirties the section anyway
 // (World::doLightingUpdates -> setBlocksDirty -> markDirty -> invalidate).
 constexpr float kNearDirtyDistSq = 32.0f * 32.0f;
 constexpr std::size_t kNearDirtyCap = 64;
 if(nearDirtyChunks_.size() < kNearDirtyCap) {
  const net::minecraft::Entity* camera =
      scene_.camera != nullptr ? scene_.camera : (scene_.client != nullptr ? scene_.client->camera : nullptr);
  if(camera != nullptr && chunk->squaredDistanceTo(camera->x, camera->y, camera->z) <= kNearDirtyDistSq) {
   nearDirtyChunks_.insert(chunk);
  }
 }
 dirtyChunks_.insert(chunk);
}
void ChunkCompilePipeline::releaseSection(chunk::ChunkBuilder& section) {
 // No deferred-retirement queue: jobs hold a weak_ptr, so the section can be
 // torn down here and now. Freeing the GL buffers on the main thread at the
 // moment of eviction is the whole point — it is the only thread that may.
 dirtyChunks_.erase(&section);
 nearDirtyChunks_.erase(&section);
 section.freeGpuBuffers();
}
bool ChunkCompilePipeline::startMeshJob(chunk::ChunkBuilder* chunk,
                                        bool nearLane,
                                        int priority,
                                        const client::option::RenderSettings& resolvedOpts) {
 if(chunk == nullptr || chunk->meshJobInFlight || !chunk->dirty) {
  return false;
 }
 auto job = chunk::ChunkMeshJob::capture(*chunk, resolvedOpts);
 if(job == nullptr) {
  return false;
 }
  chunk->meshJobInFlight = true;
 dirtyChunks_.erase(chunk);
 const auto enqueue = [&](std::shared_ptr<chunk::ChunkMeshJob> job, int jobPriority) {
  meshHandoff_.enqueue(
      std::move(job),
      [](chunk::ChunkMeshJob& meshJob) {
       try {
        chunk::ChunkBuilder::buildMesh(meshJob);
       } catch(...) {
        meshJob.failed = true;
       }
      },
      jobPriority);
 };
 if(nearLane) {
  job->nearLane = true;
  enqueue(std::move(job), std::numeric_limits<int>::min());
 } else {
  enqueue(std::move(job), priority - kMeshBias);
 }
 return true;
}
bool ChunkCompilePipeline::compileChunks(net::minecraft::entity::LivingEntity& /*camera*/, bool force) {
 sectionSystem_->drainBorderRefresh();
 const client::option::RenderSettings& resolvedOpts = *scene_.settings;
 const float gridAreaScale = static_cast<float>(sectionSystem_->renderRadiusChunks() *
                                                sectionSystem_->renderRadiusChunks()) /
                             static_cast<float>(kBaselineRadius * kBaselineRadius);
 const std::size_t workerCount = meshHandoff_.workerCount();
 const std::size_t backlog = dirtyChunks_.size() + pendingMeshUploads_.size() + meshHandoff_.pendingJobs();
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
 deferredUploads.reserve(pendingMeshUploads_.size() + meshHandoff_.pendingJobs());
 const auto processUpload = [&](std::shared_ptr<chunk::ChunkMeshJob> job, bool nearLane) {
  // Evicted while the job was in flight: the section and its buffers are
  // already gone, and this result has nowhere to land.
  const std::shared_ptr<chunk::ChunkBuilder> owner = job->builder.lock();
  if(owner == nullptr) {
   return;
  }
  chunk::ChunkBuilder* builder = owner.get();
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
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : meshHandoff_.drainCompleted()) {
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
 const std::size_t targetInFlight = workerCount * (force ? 6u : 3u);
 std::size_t inFlight = meshHandoff_.pendingJobs();
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
   if(startMeshJob(section, true, 0, resolvedOpts)) {
    it = nearDirtyChunks_.erase(it);
    ++inFlight;
    ++captures;
   } else {
    ++it;
   }
  }
  // Ascending graph-distance order (ChunkSectionSystem::rebuildSectionOrder):
  // strictly nearest-first, and unlike the old chebyshev ring buckets it
  // covers sections the occlusion walk can't reach too, so a dirty section
  // behind solid terrain still gets scheduled instead of being invisible to
  // this loop until it happens to become visible.
  const auto& sectionsByPriority = sectionSystem_->sectionsByPriority();
  for(chunk::ChunkBuilder* section : sectionsByPriority) {
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
   if(startMeshJob(section, false, section->meshPriority, resolvedOpts)) {
    ++inFlight;
    ++captures;
   }
  }
 }
 return dirtyChunks_.empty() && nearDirtyChunks_.empty() && pendingMeshUploads_.empty() && meshHandoff_.idle();
}
} // namespace net::minecraft::client::render
