#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/chunk/ChunkRegionManager.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
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
  renderX = this->x & 0x3FF;
  renderY = this->y;
  renderZ = this->z & 0x3FF;
  cameraOffsetX = this->x - renderX;
  cameraOffsetY = this->y - renderY;
  cameraOffsetZ = this->z - renderZ;
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
   for(int layer = 0; layer < 2; ++layer) {
    region_->layers[static_cast<std::size_t>(layer)].release(regionSlots_[static_cast<std::size_t>(layer)]);
   }
  }
  freeModMeshGpuBuffers();
 }
 void freeModMeshGpuBuffers() noexcept {
  for(int layer = 0; layer < 2; ++layer) {
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
  const bool modEmpty = modLayerMeshes_[0].empty() && modLayerMeshes_[1].empty();
  return renderLayerEmpty[0] && renderLayerEmpty[1] && modEmpty;
 }
 void invalidate() noexcept {
  dirty = true;
  ++version;
 }
 World* world = nullptr;
 ChunkRegionManager* regionManager_ = nullptr;
 ChunkRegion* region_ = nullptr;
 std::array<ChunkRegionBuffer::Slot, 2> regionSlots_{};
 inline static int chunkUpdates = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 int sizeX = 0;
 int sizeY = 0;
 int sizeZ = 0;
 int cameraOffsetX = 0;
 int cameraOffsetY = 0;
 int cameraOffsetZ = 0;
 int renderX = 0;
 int renderY = 0;
 int renderZ = 0;
 bool inFrustum = true;
 std::array<bool, 2> renderLayerEmpty{true, true};
 std::array<std::vector<ModChunkMesh>, 2> modLayerMeshes_{};
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
} // namespace net::minecraft::client::render::chunk
