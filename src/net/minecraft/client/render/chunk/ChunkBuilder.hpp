#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshJob.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::render::chunk {
inline constexpr int kSectionBlocks = 16;
struct ModChunkMesh;
struct LayerVbo {
 unsigned handle = 0;
 int vertexCount = 0;
 [[nodiscard]] bool valid() const noexcept { return handle != 0; }
};
class ChunkBuilder : public std::enable_shared_from_this<ChunkBuilder> {
 public:
 ChunkBuilder(World* world,
              std::vector<::net::minecraft::block::entity::BlockEntity*>& blockEntityUpdateList,
              int x,
              int y,
              int z,
              int size)
     : world(world), x(x), y(y), z(z), currentBlockEntities_(&blockEntityUpdateList) {
  centerX = this->x + size / 2;
  centerY = this->y + size / 2;
  centerZ = this->z + size / 2;
  constexpr float padding = 6.0f;
  cullingBox = net::minecraft::Box(static_cast<double>(this->x) - padding,
                                   static_cast<double>(this->y) - padding,
                                   static_cast<double>(this->z) - padding,
                                   static_cast<double>(this->x + size) + padding,
                                   static_cast<double>(this->y + size) + padding,
                                   static_cast<double>(this->z + size) + padding);
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
 void drawLayer(int layer) const;
 void freeGpuBuffers() noexcept;
 void freeModMeshGpuBuffers() noexcept {
  for(int layer = 0; layer < terrain_layer::Count; ++layer) {
   for(ModChunkMesh& modMesh : modLayerMeshes_[static_cast<std::size_t>(layer)]) {
    modMesh.mesh.freeGpuBuffer();
   }
  }
 }
 void updateFrustum(const Frustum& culler) {
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
 std::array<LayerVbo, terrain_layer::Count> layerVbos_{};
 inline static int frameDrawCalls = 0;
 inline static int chunkUpdates = 0;
 int x = 0;
 int y = 0;
 int z = 0;
 bool inFrustum = true;
 std::array<bool, terrain_layer::Count> renderLayerEmpty{true, true, true, true};
 std::array<std::vector<ModChunkMesh>, terrain_layer::Count> modLayerMeshes_{};
 int centerX = 0;
 int centerY = 0;
 int centerZ = 0;
 bool dirty = false;
 net::minecraft::Box cullingBox{0, 0, 0, 0, 0, 0};
 int meshPriority = 0;
 int meshOrderStamp = -1;
 bool hasSkyLight = false;
 bool built = false;
 int version = 0;
 bool meshJobInFlight = false;
 struct OcclusionState {
  std::uint64_t visBits = ~0ULL;
  int stamp = -1;
  int entryFace = -1;
  [[nodiscard]] bool visitedIn(int walkStamp) const noexcept {
   return stamp == walkStamp;
  }
  void enter(int walkStamp, int face) noexcept {
   stamp = walkStamp;
   entryFace = face;
  }
  [[nodiscard]] bool connects(int face, bool built) const noexcept {
   if(entryFace < 0 || !built) {
    return true;
   }
   return (visBits & (1ULL << (entryFace * 6 + face))) != 0;
  }
 };
  OcclusionState occlusion{};
  ChunkBuilder* neighbors[6] = {};
  std::vector<::net::minecraft::block::entity::BlockEntity*> blockEntities_{};
 std::vector<::net::minecraft::block::entity::BlockEntity*>* currentBlockEntities_ = nullptr;
};
} // namespace net::minecraft::client::render::chunk
