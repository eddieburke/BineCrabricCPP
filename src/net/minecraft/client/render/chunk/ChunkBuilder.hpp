#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/chunk/TerrainRegion.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::render::chunk {
inline constexpr int kSectionBlocks = 16;
struct ModChunkMesh;
class ChunkBuilder : public std::enable_shared_from_this<ChunkBuilder> {
 public:
 ChunkBuilder(World* world,
              std::vector<::net::minecraft::block::entity::BlockEntity*>& blockEntityUpdateList,
              int x,
              int y,
              int z)
     : world(world), x(x), y(y), z(z), currentBlockEntities_(&blockEntityUpdateList) {
  centerX = this->x + kSectionBlocks / 2;
  centerY = this->y + kSectionBlocks / 2;
  centerZ = this->z + kSectionBlocks / 2;
  dirty = false;
 }
 // Derived, not stored: a fixed offset from x/y/z. kCullPadding matches the region
 // box in ChunkSectionSystem, which has to contain every section box inside it.
 static constexpr double kCullPadding = 6.0;
 [[nodiscard]] net::minecraft::Box cullingBounds() const noexcept {
  return net::minecraft::Box(static_cast<double>(x) - kCullPadding, static_cast<double>(y) - kCullPadding,
                             static_cast<double>(z) - kCullPadding,
                             static_cast<double>(x + kSectionBlocks) + kCullPadding,
                             static_cast<double>(y + kSectionBlocks) + kCullPadding,
                             static_cast<double>(z + kSectionBlocks) + kCullPadding);
 }
 [[nodiscard]] float squaredDistanceTo(double entityX, double entityY, double entityZ) const {
  const float dx = static_cast<float>(entityX - static_cast<double>(centerX));
  const float dy = static_cast<float>(entityY - static_cast<double>(centerY));
  const float dz = static_cast<float>(entityZ - static_cast<double>(centerZ));
  return dx * dx + dy * dy + dz * dz;
 }
 static void buildMesh(ChunkMeshJob& job);
 void uploadMesh(ChunkMeshJob& job);
 void freeGpuBuffers() noexcept;
 void setTerrainRegion(TerrainRegion& region) noexcept { terrainRegion_ = &region; }
 [[nodiscard]] TerrainRegion* terrainRegion() const noexcept { return terrainRegion_; }
 [[nodiscard]] const TerrainAllocation& terrainAllocation(int layer) const noexcept {
  return terrainAllocations_[static_cast<std::size_t>(layer)];
 }
 void freeModMeshGpuBuffers() noexcept {
  for(int layer = 0; layer < terrain_layer::Count; ++layer) {
   for(ModChunkMesh& modMesh : modLayerMeshes_[static_cast<std::size_t>(layer)]) {
    modMesh.mesh.freeGpuBuffer();
   }
  }
 }
 // Visibility is the stamp, not a flag beside one. cullChunks no longer sweeps
 // every resident section to clear a bool, so a section the walk never reached
 // simply keeps an older stamp -- there is no stale "true" to read.
 [[nodiscard]] bool updateFrustum(const Frustum& culler, int stamp) {
  if(!culler.isVisible(static_cast<double>(x) - kCullPadding, static_cast<double>(y) - kCullPadding,
                       static_cast<double>(z) - kCullPadding, static_cast<double>(x + kSectionBlocks) + kCullPadding,
                       static_cast<double>(y + kSectionBlocks) + kCullPadding,
                       static_cast<double>(z + kSectionBlocks) + kCullPadding)) {
   return false;
  }
  visibleStamp = stamp;
  return true;
 }
 [[nodiscard]] bool visibleIn(int stamp) const noexcept {
  return visibleStamp == stamp;
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
 [[nodiscard]] bool readyForMeshCapture() const noexcept {
  return lightingReady && dirty && !meshJobInFlight;
 }
 World* world = nullptr;
 std::array<TerrainAllocation, terrain_layer::Count> terrainAllocations_{};
 inline static int frameDrawCalls = 0;
 inline static int chunkUpdates = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 int visibleStamp = -1;
 std::array<bool, terrain_layer::Count> renderLayerEmpty{true, true, true, true};
 std::array<std::vector<ModChunkMesh>, terrain_layer::Count> modLayerMeshes_{};
 int centerX = 0;
 int centerY = 0;
 int centerZ = 0;
 bool dirty = false;
 bool hasSkyLight = false;
 bool lightingReady = true;
 bool built = false;
 int version = 0;
 bool meshJobInFlight = false;
 struct OcclusionState {
  std::uint64_t visBits = ~0ULL;
  int stamp = -1;
  std::uint8_t entryFaces = 0;
  std::uint8_t expandedFaces = 0;
  [[nodiscard]] bool enter(int walkStamp, int face) noexcept {
   if(stamp != walkStamp) {
    stamp = walkStamp;
    entryFaces = 0;
    expandedFaces = 0;
   }
   const std::uint8_t bit = static_cast<std::uint8_t>(1U << (face < 0 ? 6 : face));
   if((entryFaces & bit) != 0) {
    return false;
   }
   entryFaces = static_cast<std::uint8_t>(entryFaces | bit);
   return true;
  }
  [[nodiscard]] bool connects(int entryFace, int exitFace, bool built) const noexcept {
   if(entryFace < 0 || !built) {
    return true;
   }
   return (visBits & (1ULL << (entryFace * 6 + exitFace))) != 0;
  }
  [[nodiscard]] bool claimExit(int walkStamp, int entryFace, int exitFace, bool built) noexcept {
   if(stamp != walkStamp || !connects(entryFace, exitFace, built)) {
    return false;
   }
   const std::uint8_t bit = static_cast<std::uint8_t>(1U << exitFace);
   if((expandedFaces & bit) != 0) {
    return false;
   }
   expandedFaces = static_cast<std::uint8_t>(expandedFaces | bit);
   return true;
  }
 };
  OcclusionState occlusion{};
  ChunkBuilder* neighbors[6] = {};
  std::vector<::net::minecraft::block::entity::BlockEntity*> blockEntities_{};
 std::vector<::net::minecraft::block::entity::BlockEntity*>* currentBlockEntities_ = nullptr;
 TerrainRegion* terrainRegion_ = nullptr;
};
} // namespace net::minecraft::client::render::chunk
