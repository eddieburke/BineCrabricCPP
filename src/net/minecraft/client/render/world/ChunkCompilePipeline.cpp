#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/debug/VTuneTrace.hpp"
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
constexpr int kMeshBias = 4;
} // namespace
void ChunkCompilePipeline::enqueueDirtyChunk(chunk::ChunkBuilder* chunk) {
 if(chunk == nullptr || chunk->meshJobInFlight) {
  return;
 }
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
 if(chunk == nullptr || !chunk->readyForMeshCapture()) {
  return false;
 }
 const auto captureStart = std::chrono::steady_clock::now();
 auto job = chunk::ChunkMeshJob::capture(*chunk, resolvedOpts);
 if(job == nullptr) {
  return false;
 }
 job->captureNs = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                 std::chrono::steady_clock::now() - captureStart)
                                                 .count());
 chunk->meshJobInFlight = true;
 dirtyChunks_.erase(chunk);
 const auto enqueue = [&](std::shared_ptr<chunk::ChunkMeshJob> job, int jobPriority) {
  meshHandoff_.enqueue(
      std::move(job),
      [](chunk::ChunkMeshJob& meshJob) {
       const auto buildStart = std::chrono::steady_clock::now();
       try {
        chunk::ChunkBuilder::buildMesh(meshJob);
       } catch(...) {
        meshJob.failed = true;
       }
       meshJob.buildNs = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                        std::chrono::steady_clock::now() - buildStart)
                                                        .count());
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
 VT_TRACE_EVENT("terrain/compile");
 {
  VT_TRACE_EVENT("terrain/border_refresh");
  sectionSystem_->drainBorderRefresh();
 }
 const client::option::RenderSettings& resolvedOpts = *scene_.settings;
 const std::size_t workerCount = meshHandoff_.workerCount();
 const std::size_t backlog = dirtyChunks_.size() + pendingMeshUploads_.size() + meshHandoff_.pendingJobs();
 VT_TRACE_COUNTER("MeshJobsQueued", dirtyChunks_.size());
 VT_TRACE_COUNTER("MeshJobsInFlight", meshHandoff_.pendingJobs());
 VT_TRACE_COUNTER("MeshUploadsPending", pendingMeshUploads_.size());
#ifdef VTUNE_ENABLED
 const std::size_t lightingHeldSections = static_cast<std::size_t>(std::count_if(
     dirtyChunks_.begin(), dirtyChunks_.end(), [](const chunk::ChunkBuilder* section) {
      return section != nullptr && section->dirty && !section->lightingReady;
     }));
 VT_TRACE_COUNTER("LightingMeshSectionsHeld", lightingHeldSections);
 VT_TRACE_COUNTER("MeshJobsStaleTotal", staleMeshJobs_);
 VT_TRACE_COUNTER("MeshUploadsDeferredTotal", deferredMeshUploads_);
#endif
 const bool loadingBacklog = backlog > 512u;
 const int minUploadsPerFrame = loadingBacklog ? std::clamp(static_cast<int>(workerCount * 2u), 4, 16) : 1;
 const net::minecraft::util::concurrent::FrameBudget uploadBudget =
     net::minecraft::util::concurrent::FrameBudget::fromSharedMs(loadingBacklog ? 6 : 3, minUploadsPerFrame);
 // Near-camera edits get a dedicated slice (QD-21): they are uploaded before
 // the loading backlog so a block edit next to the player lands the frame its
 // mesh finishes, not when background work's budget allows (HZ-31).
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
  if(!job->profileRecorded) {
   VT_TRACE_COUNTER("mesh_capture_ns", job->captureNs);
   VT_TRACE_COUNTER("mesh_build_worker_ns", job->buildNs);
   job->profileRecorded = true;
  }
  const net::minecraft::util::concurrent::FrameBudget& budget = nearLane ? nearBudget : uploadBudget;
  const int budgetCount = nearLane ? nearUploadCount : uploadCount;
  if(!budget.hasRemaining(budgetCount)) {
   ++deferredMeshUploads_;
   deferredUploads.push_back(std::move(job));
   return;
  }
  builder->meshJobInFlight = false;
  if(job->failed || job->version != builder->version) {
   ++staleMeshJobs_;
   builder->dirty = true;
   enqueueDirtyChunk(builder);
   return;
  }
  std::uint64_t uploadBytes = 0;
  for(const TessellatorMesh& layer : job->result.layers) {
   uploadBytes += layer.vertexCount() * sizeof(TessellatorVertex);
  }
  VT_TRACE_COUNTER("MeshUploadBytes", uploadBytes);
  {
   VT_TRACE_EVENT("terrain/mesh_upload");
   builder->uploadMesh(*job);
  }
  builder->dirty = false;
  dirtyChunks_.erase(builder);
  if(nearLane) {
   ++nearUploadCount;
  } else {
   ++uploadCount;
  }
 };
 std::vector<std::shared_ptr<chunk::ChunkMeshJob>> nearUploads;
 std::vector<std::shared_ptr<chunk::ChunkMeshJob>> backgroundUploads;
 nearUploads.reserve(pendingMeshUploads_.size());
 backgroundUploads.reserve(pendingMeshUploads_.size());
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : pendingMeshUploads_) {
  if(job->nearLane) {
   nearUploads.push_back(std::move(job));
  } else {
   backgroundUploads.push_back(std::move(job));
  }
 }
 pendingMeshUploads_.clear();
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : meshHandoff_.drainCompleted()) {
  if(job->nearLane) {
   nearUploads.push_back(std::move(job));
  } else {
   backgroundUploads.push_back(std::move(job));
  }
 }
 {
  for(std::shared_ptr<chunk::ChunkMeshJob>& job : nearUploads) {
   processUpload(std::move(job), true);
  }
  for(std::shared_ptr<chunk::ChunkMeshJob>& job : backgroundUploads) {
   processUpload(std::move(job), false);
  }
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
  // Walk the dirty set, not every resident section. The old loop scanned all of
  // sectionsByPriority() and hashed each one against dirtyChunks_ every frame --
  // order-of-view-distance lookups even with nothing dirty at all.
  if(!dirtyChunks_.empty() && canCapture()) {
   const int frustumStamp = sectionSystem_->frustumStamp();
   captureCandidates_.clear();
   captureCandidates_.reserve(dirtyChunks_.size());
   for(auto it = dirtyChunks_.begin(); it != dirtyChunks_.end();) {
    chunk::ChunkBuilder* section = *it;
    if(section == nullptr) {
     it = dirtyChunks_.erase(it);
     continue;
    }
    if(!section->dirty) {
     it = dirtyChunks_.erase(it);
     continue;
    }
    ++it;
    if(section->meshJobInFlight || (force && !section->visibleIn(frustumStamp))) {
     continue;
    }
    captureCandidates_.push_back(section);
   }
   // Only the few we can actually start this frame need to be in priority order,
   // so the nearest dirty sections are tried first.
   const std::size_t wanted =
       std::min(captureCandidates_.size(), inFlight < targetInFlight ? targetInFlight - inFlight : 0u);
   std::partial_sort(captureCandidates_.begin(),
                     captureCandidates_.begin() + static_cast<std::ptrdiff_t>(wanted),
                     captureCandidates_.end(),
                     [](const chunk::ChunkBuilder* a, const chunk::ChunkBuilder* b) {
                      return a->meshPriority < b->meshPriority;
                     });
   // Walk past `wanted`: startMeshJob refuses a section whose neighbours are
   // evicted, and a refusal costs no budget. Stopping at `wanted` would let a run
   // of refusals end the frame's capture early and leave dirty sections showing
   // stale lighting until some later frame happened to reach them.
   for(chunk::ChunkBuilder* section : captureCandidates_) {
    if(!canCapture()) {
     break;
    }
    if(startMeshJob(section, false, section->meshPriority, resolvedOpts)) {
     ++inFlight;
     ++captures;
    }
   }
  }
 }
 return dirtyChunks_.empty() && nearDirtyChunks_.empty() && pendingMeshUploads_.empty() && meshHandoff_.idle();
}
} // namespace net::minecraft::client::render
