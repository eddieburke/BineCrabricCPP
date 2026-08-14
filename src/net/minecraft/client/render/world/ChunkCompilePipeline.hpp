#pragma once
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/world/TerrainScene.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/util/concurrent/WorkerHandoff.hpp"
namespace net::minecraft::entity {
class LivingEntity;
}
namespace net::minecraft::client::render {
class ChunkSectionSystem;
// Owns the mesh build/upload pipeline of the world renderer: the dirty-section
// sets, the shared mesh handoff and the per-frame upload/capture FrameBudget
// slicing in compileChunks.
class ChunkCompilePipeline {
 public:
 explicit ChunkCompilePipeline(TerrainScene& scene) : scene_(scene) {
 }
 // Peer reference, wired by WorldRenderer after both systems exist. Their
 // construction is mutually circular — each needs the other's services — so
 // each is built against the scene alone and the owner then supplies the peer.
 void setSectionSystem(ChunkSectionSystem& system) noexcept {
  sectionSystem_ = &system;
 }
 bool compileChunks(net::minecraft::entity::LivingEntity& camera, bool force);
 void enqueueDirtyChunk(chunk::ChunkBuilder* chunk);
 bool startMeshJob(chunk::ChunkBuilder* chunk,
                   bool nearLane,
                   int priority,
                   const client::option::RenderSettings& resolvedOpts);
 // Drop a section's tracking and GL buffers. Call while the owner still holds
 // it; the shared_ptr may then be released immediately.
 void releaseSection(chunk::ChunkBuilder& section);
 void cancelAll() {
  meshHandoff_.cancelAll();
  pendingMeshUploads_.clear();
 }
 void clearDirtyTracking() {
  dirtyChunks_.clear();
  nearDirtyChunks_.clear();
 }

 private:
 // Runs ChunkBuilder::buildMesh on the shared compute pool and hands finished
 // jobs back to the main thread. Near-camera edits and the distance backlog
 // share one priority queue so edits jump ahead of queued distant work.
 net::minecraft::util::concurrent::WorkerHandoff<chunk::ChunkMeshJob> meshHandoff_{
     net::minecraft::util::concurrent::ThreadCoordinator::instance().pool(
         net::minecraft::util::concurrent::Domain::Compute)};
 TerrainScene& scene_;
 ChunkSectionSystem* sectionSystem_ = nullptr;
 std::unordered_set<chunk::ChunkBuilder*> dirtyChunks_{};
 std::unordered_set<chunk::ChunkBuilder*> nearDirtyChunks_{};
 std::vector<std::shared_ptr<chunk::ChunkMeshJob>> pendingMeshUploads_{};
 std::uint64_t staleMeshJobs_ = 0;
 std::uint64_t deferredMeshUploads_ = 0;
};
} // namespace net::minecraft::client::render
