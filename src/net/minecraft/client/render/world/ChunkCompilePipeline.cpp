#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/chunk/MeshUploadBudget.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include <limits>
namespace net::minecraft::client::render {
namespace {
constexpr int kMeshBias = 4;
constexpr int kAmbientBacklogCeiling = 448;
constexpr int kAmbientDrainPerFrame = 256;
} // namespace
void ChunkCompilePipeline::prioritizeCaptureCandidates(std::vector<chunk::ChunkBuilder*>& candidates,
                                                       int frustumStamp,
                                                       int cameraSectionX,
                                                       int cameraSectionY,
                                                       int cameraSectionZ,
                                                       std::size_t wanted) {
 const auto distance = [cameraSectionX, cameraSectionY, cameraSectionZ](const chunk::ChunkBuilder* section) {
  return std::abs((section->x >> 4) - cameraSectionX) + std::abs((section->y >> 4) - cameraSectionY) +
         std::abs((section->z >> 4) - cameraSectionZ);
 };
 const auto firstInvisible = std::partition(candidates.begin(), candidates.end(), [frustumStamp](const auto* section) {
  return section->visibleIn(frustumStamp);
 });
 const std::size_t visibleCount = static_cast<std::size_t>(firstInvisible - candidates.begin());
 const std::size_t visibleWanted = std::min(wanted, visibleCount);
 const auto nearFirst = [&distance](const auto* a, const auto* b) {
  return a->built != b->built ? !a->built : distance(a) < distance(b);
 };
 std::partial_sort(candidates.begin(),
                   candidates.begin() + static_cast<std::ptrdiff_t>(visibleWanted),
                   firstInvisible,
                   nearFirst);
 const std::size_t invisibleCount = static_cast<std::size_t>(candidates.end() - firstInvisible);
 const std::size_t invisibleWanted = std::min(wanted, invisibleCount);
 std::partial_sort(firstInvisible,
                   firstInvisible + static_cast<std::ptrdiff_t>(invisibleWanted),
                   candidates.end(),
                   nearFirst);
}
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
void ChunkCompilePipeline::eraseDirtyChunk(chunk::ChunkBuilder* section) {
 dirtyChunks_.erase(section);
}
void ChunkCompilePipeline::gatherCaptureCandidates(bool force) {
 captureCandidates_.clear();
 captureCandidates_.reserve(dirtyChunks_.size());
 const int stamp = sectionSystem_->frustumStamp();
 for(auto it = dirtyChunks_.begin(); it != dirtyChunks_.end();) {
  chunk::ChunkBuilder* section = *it;
  if(section == nullptr || !section->dirty) {
   it = dirtyChunks_.erase(it);
   continue;
  }
  ++it;
  if(section->readyForMeshCapture() && (!force || section->visibleIn(stamp))) {
   captureCandidates_.push_back(section);
  }
 }
}
void ChunkCompilePipeline::releaseSection(chunk::ChunkBuilder& section) {
 // No deferred-retirement queue: jobs hold a weak_ptr, so the section can be
 // torn down here and now. Freeing the GL buffers on the main thread at the
 // moment of eviction is the whole point — it is the only thread that may.
 eraseDirtyChunk(&section);
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
 auto job = chunk::ChunkMeshJob::capture(*chunk, resolvedOpts);
 if(job == nullptr) {
  return false;
 }
 chunk->meshJobInFlight = true;
 eraseDirtyChunk(chunk);
 job->nearLane = nearLane;
 meshHandoff_.enqueue(
      std::move(job),
      [](chunk::ChunkMeshJob& meshJob) {
       try {
        meshJob.buildSnapshot();
        chunk::ChunkBuilder::buildMesh(meshJob);
       } catch(...) {
        meshJob.failed = true;
       }
       meshJob.snapshot.reset();
      },
      nearLane ? std::numeric_limits<int>::min() : priority - kMeshBias);
 return true;
}
bool ChunkCompilePipeline::compileChunks(net::minecraft::entity::LivingEntity& camera, bool force) {
 sectionSystem_->drainBorderRefresh();
 if(force) {
  sectionSystem_->drainAmbientInvalidations(std::numeric_limits<int>::max());
 } else {
  const std::size_t queued = dirtyChunks_.size() + pendingMeshUploads_.size() + meshHandoff_.pendingJobs();
  const int ambientAllowance =
      std::clamp(kAmbientBacklogCeiling - static_cast<int>(queued), 0, kAmbientDrainPerFrame);
  if(ambientAllowance > 0) {
   sectionSystem_->drainAmbientInvalidations(ambientAllowance);
  }
 }
 const client::option::RenderSettings& resolvedOpts = *scene_.settings;
 const std::size_t workerCount = meshHandoff_.workerCount();
 const int requestedWork = client::option::chunkUpdatesPerPass(resolvedOpts, static_cast<int>(dirtyChunks_.size()));
 int uploadCount = 0;
 chunk::MeshUploadBudget byteBudget;
 const auto processUpload = [&](const std::shared_ptr<chunk::ChunkMeshJob>& job) {
  // Evicted while the job was in flight: the section and its buffers are
  // already gone, and this result has nowhere to land.
  const std::shared_ptr<chunk::ChunkBuilder> owner = job->builder.lock();
  if(owner == nullptr) {
   return true;
  }
  chunk::ChunkBuilder* builder = owner.get();
  if(job->failed || job->version != builder->version) {
   builder->meshJobInFlight = false;
   builder->dirty = true;
   enqueueDirtyChunk(builder);
   return true;
  }
  std::size_t bytes = 0;
  for(int layer = 0; layer < chunk::terrain_layer::Count; ++layer) {
   bytes += job->result.layers[layer].vertices.size() * sizeof(TerrainVertex);
   for(const auto& mesh : job->result.modLayers[layer]) bytes += mesh.mesh.vertices.size() * sizeof(TessellatorVertex);
  }
  if(uploadCount >= requestedWork || !byteBudget.tryConsume(bytes)) return false;
  builder->meshJobInFlight = false;
  ++uploadCount;
  if(!builder->uploadMesh(*job)) {
   builder->dirty = true;
   enqueueDirtyChunk(builder);
   return true;
  }
  builder->dirty = false;
  eraseDirtyChunk(builder);
  return true;
 };
 meshHandoff_.drainCompletedInto(completedMeshUploads_);
 for(std::shared_ptr<chunk::ChunkMeshJob>& job : completedMeshUploads_) {
  pendingMeshUploads_.push_back(std::move(job));
 }
 completedMeshUploads_.clear();
 for(const bool nearLane : {true, false}) {
  for(std::shared_ptr<chunk::ChunkMeshJob>& job : pendingMeshUploads_) {
   if(job != nullptr && job->nearLane == nearLane && processUpload(job)) job.reset();
  }
 }
 std::erase(pendingMeshUploads_, nullptr);
 const std::size_t targetInFlight = workerCount * (force ? 6u : 3u);
 std::size_t inFlight = meshHandoff_.pendingJobs();
 std::size_t pendingCpuBytes = 0;
 for(const auto& job : pendingMeshUploads_) {
  for(int layer = 0; layer < chunk::terrain_layer::Count; ++layer) {
   pendingCpuBytes += job->result.layers[layer].vertices.capacity() * sizeof(TessellatorVertex);
   for(const auto& mesh : job->result.modLayers[layer])
    pendingCpuBytes += mesh.mesh.vertices.capacity() * sizeof(TessellatorVertex);
  }
 }
 if(inFlight < targetInFlight && pendingMeshUploads_.size() < targetInFlight * 2u &&
    pendingCpuBytes < 64u * 1024u * 1024u) {
  const int maxCaptures = force ? std::max(requestedWork, static_cast<int>(workerCount)) : requestedWork;
  int captures = 0;
  const auto canCapture = [&] { return captures < maxCaptures && inFlight < targetInFlight; };
  for(auto it = nearDirtyChunks_.begin(); it != nearDirtyChunks_.end() && canCapture();) {
   chunk::ChunkBuilder* section = *it;
   if(section == nullptr || !section->dirty || section->meshJobInFlight) {
    it = nearDirtyChunks_.erase(it);
    continue;
   }
   if(!section->readyForMeshCapture()) {
    ++it;
    continue;
   }
   const bool started = startMeshJob(section, true, 0, resolvedOpts);
   it = nearDirtyChunks_.erase(it);
   if(started) {
    ++inFlight;
    ++captures;
   }
  }
  // Walk the dirty set, not every resident section: an order-of-view-distance sweep
  // even with nothing dirty at all.
  if(!dirtyChunks_.empty() && canCapture()) {
   const int frustumStamp = sectionSystem_->frustumStamp();
   // Manhattan distance in sections from the camera reproduces what a breadth-first walk
   // over the 6-neighbour section graph assigned, without touching every resident section
   // twice on every frame that created a column.
   const int camSectionX = MathHelper::floor(camera.x) >> 4;
   const int camSectionY = MathHelper::floor(camera.y) >> 4;
   const int camSectionZ = MathHelper::floor(camera.z) >> 4;
   const auto meshPriority = [camSectionX, camSectionY, camSectionZ](const chunk::ChunkBuilder* section) {
    return std::abs((section->x >> 4) - camSectionX) + std::abs((section->y >> 4) - camSectionY) +
           std::abs((section->z >> 4) - camSectionZ);
   };
   gatherCaptureCandidates(force);
   // Only the few we can actually start this frame need to be in priority order,
   // so the nearest dirty sections are tried first.
   const std::size_t wanted = std::min({captureCandidates_.size(), targetInFlight - inFlight,
                                       static_cast<std::size_t>(maxCaptures - captures)});
   prioritizeCaptureCandidates(
       captureCandidates_, frustumStamp, camSectionX, camSectionY, camSectionZ, wanted);
   // Walk past `wanted`: startMeshJob refuses a section whose neighbours are
   // evicted, and a refusal costs no budget. Stopping at `wanted` would let a run
   // of refusals end the frame's capture early and leave dirty sections showing
   // stale lighting until some later frame happened to reach them.
   for(chunk::ChunkBuilder* section : captureCandidates_) {
    if(!canCapture()) {
     break;
    }
    if(startMeshJob(section, false, meshPriority(section), resolvedOpts)) {
     ++inFlight;
     ++captures;
    }
   }
  }
 }
 return dirtyChunks_.empty() && nearDirtyChunks_.empty() && pendingMeshUploads_.empty() && meshHandoff_.idle();
}
} // namespace net::minecraft::client::render
