#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/chunk/ChunkRegionBuffer.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/util/concurrent/WorkerHandoff.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::render::chunk {
struct ModChunkMesh;
class ChunkBuilder {
 public:
 ChunkBuilder(World* world,
              std::vector<::net::minecraft::block::entity::BlockEntity*>& blockEntityUpdateList,
              int x,
              int y,
              int z,
              int size,
              ChunkRegionManager* regionManager)
     : world(world), regionManager_(regionManager), x(x), y(y), z(z), currentBlockEntities_(&blockEntityUpdateList) {
  sizeX = size;
  sizeY = size;
  sizeZ = size;
  radius = std::sqrt(static_cast<float>(sizeX * sizeX + sizeY * sizeY + sizeZ * sizeZ)) / 2.0f;
  centerX = this->x + sizeX / 2;
  centerY = this->y + sizeY / 2;
  centerZ = this->z + sizeZ / 2;
  constexpr float padding = 6.0f;
  cullingBox = net::minecraft::Box(static_cast<double>(this->x) - padding,
                                   static_cast<double>(this->y) - padding,
                                   static_cast<double>(this->z) - padding,
                                   static_cast<double>(this->x + sizeX) + padding,
                                   static_cast<double>(this->y + sizeY) + padding,
                                   static_cast<double>(this->z + sizeZ) + padding);
  dirty = false;
 }
 [[nodiscard]] float squaredDistanceTo(double entityX, double entityY, double entityZ) const {
  const float dx = static_cast<float>(entityX - static_cast<double>(centerX));
  const float dy = static_cast<float>(entityY - static_cast<double>(centerY));
  const float dz = static_cast<float>(entityZ - static_cast<double>(centerZ));
  return dx * dx + dy * dy + dz * dz;
 }
 static void buildMesh(ChunkMeshJob& job);
 void uploadMesh(ChunkMeshJob& job);
 void freeRegionSlots() noexcept {
  if(region_ != nullptr) {
   for(int layer = 0; layer < terrain_layer::Count; ++layer) {
    region_->layers[static_cast<std::size_t>(layer)].release(regionSlots_[static_cast<std::size_t>(layer)]);
   }
  }
  freeModMeshGpuBuffers();
 }
 void freeModMeshGpuBuffers() noexcept {
  for(int layer = 0; layer < terrain_layer::Count; ++layer) {
   for(ModChunkMesh& modMesh : modLayerMeshes_[static_cast<std::size_t>(layer)]) {
    modMesh.mesh.freeGpuBuffer();
   }
  }
 }
 void updateFrustum(const FrustumCuller& culler) {
  inFrustum = culler.isVisible(cullingBox);
 }
 [[nodiscard]] bool hasNoGeometry() const noexcept {
  if(!built) {
   return false;
  }
  bool empty = true;
  for(int layer = 0; layer < terrain_layer::Count; ++layer) {
   empty = empty && renderLayerEmpty[static_cast<std::size_t>(layer)] &&
           modLayerMeshes_[static_cast<std::size_t>(layer)].empty();
  }
  return empty;
 }
 void invalidate() noexcept {
  dirty = true;
  ++version;
 }
 World* world = nullptr;
 ChunkRegionManager* regionManager_ = nullptr;
 ChunkRegion* region_ = nullptr;
 std::array<ChunkRegionBuffer::Slot, terrain_layer::Count> regionSlots_{};
 inline static int chunkUpdates = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 int sizeX = 0;
 int sizeY = 0;
 int sizeZ = 0;
 bool inFrustum = true;
 std::array<bool, terrain_layer::Count> renderLayerEmpty{true, true, true};
 std::array<std::vector<ModChunkMesh>, terrain_layer::Count> modLayerMeshes_{};
 int centerX = 0;
 int centerY = 0;
 int centerZ = 0;
 float radius = 0.0f;
 bool dirty = false;
 net::minecraft::Box cullingBox{0, 0, 0, 0, 0, 0};
 int id = 0;
 int drawRing = 0;
 bool hasSkyLight = false;
 bool built = false;
 int version = 0;
 bool meshJobInFlight = false;
 bool retired = false;
 // Face connectivity from the last successful rebuild; faces 0:-X 1:+X 2:-Y
 // 3:+Y 4:-Z 5:+Z, opposite = face^1. occ* fields are WorldRenderer BFS
 // scratch state stamped per traversal.
 std::uint64_t visBits = ~0ULL;
 int occStamp = -1;
 int occEntryFace = -1;
 std::vector<::net::minecraft::block::entity::BlockEntity*> blockEntities_{};
 std::vector<::net::minecraft::block::entity::BlockEntity*>* currentBlockEntities_ = nullptr;
};

// Runs ChunkBuilder::buildMesh on a worker pool and hands finished jobs back to
// the main thread. Near-camera edits and the distance backlog share one priority
// queue so edits jump ahead of queued distant work.
class ChunkMeshScheduler {
 public:
 void enqueue(std::shared_ptr<ChunkMeshJob> job, int priority) {
  handoff_.enqueue(
      std::move(job),
      [](ChunkMeshJob& meshJob) {
       try {
        ChunkBuilder::buildMesh(meshJob);
       } catch(...) {
        meshJob.failed = true;
       }
      },
      priority);
 }
 void enqueueNear(std::shared_ptr<ChunkMeshJob> job) {
  job->nearLane = true;
  enqueue(std::move(job), std::numeric_limits<int>::min());
 }
 [[nodiscard]] std::vector<std::shared_ptr<ChunkMeshJob>> drainCompleted() {
  return handoff_.drainCompleted();
 }
 void cancelAll() {
  handoff_.cancelAll();
 }
 [[nodiscard]] bool idle() const {
  return handoff_.idle();
 }
 [[nodiscard]] std::size_t pendingJobs() const {
  return handoff_.pendingJobs();
 }
 [[nodiscard]] unsigned workerCount() const noexcept {
  return handoff_.workerCount();
 }

 private:
  // Mesh submits to the shared Compute pool (one pool for all compute owners);
  // completed jobs cross back on a bounded Channel. Lighting/loader keep their
  // computeShare(3) sub-pools until PASS-2 WI-6/7 join them to the shared pool.
  net::minecraft::util::concurrent::WorkerHandoff<ChunkMeshJob> handoff_{
      net::minecraft::util::concurrent::ThreadCoordinator::instance().pool(
          net::minecraft::util::concurrent::Domain::Compute)};
};
} // namespace net::minecraft::client::render::chunk
