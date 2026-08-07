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
// Sections are owned by ChunkSectionSystem through a shared_ptr and referenced
// by in-flight mesh jobs through a weak_ptr. That is the whole lifetime rule:
// a worker never touches the builder (it reads its own snapshot and writes its
// own result), and the main thread can only reach a builder that is still
// alive, because reaching it requires locking the weak_ptr.
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
 // Draw the given layer's mesh. Caller must have set up the program and the
 // pending terrain draw (chunkOffset = section origin - camera). Vertices are
 // stored section-local; the shader adds chunkOffset to reach camera space.
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
 // Per-frame draw-call accounting for the debug HUD.
 inline static int frameDrawCalls = 0;
 // Per-frame chunk mesh upload accounting for the debug HUD.
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
 // Graph-distance-from-camera, in section-adjacency steps (not blocks),
 // computed by ChunkSectionSystem::rebuildSectionOrder. Used only as a
 // relative mesh-build priority -- smaller means nearer, and
 // ChunkCompilePipeline walks sections in ascending order of this value.
 // Replaces the old chebyshev `drawRing`; see ChunkSectionSystem.hpp for why
 // graph distance beats straight-line distance here.
 int meshPriority = 0;
 // Generation stamp meshPriority was last written in. Compared against
 // ChunkSectionSystem's own counter so a section the walk never reached
 // this pass (e.g. a column still streaming in) reads as "unknown" rather
 // than being mistaken for one that is genuinely 0 steps from the camera.
 int meshOrderStamp = -1;
 bool hasSkyLight = false;
 bool built = false;
 int version = 0;
 // Only a "don't enqueue a second job for this section" flag. Builder
 // lifetime is not tied to it: a job holds a weak_ptr, so an evicted section
 // is freed immediately and a late job simply finds nothing to upload.
 bool meshJobInFlight = false;
 // Per-section state for the visibility-graph BFS, kept together so the walk
 // reads as one thing rather than three loose ints poked from another class.
 struct OcclusionState {
  // 6x6 face-to-face connectivity through non-opaque blocks (bit from*6+to).
  // All-ones until the section is built, i.e. assume see-through.
  std::uint64_t visBits = ~0ULL;
  // BFS generation this section was last reached in; compared against the
  // walk's stamp instead of clearing a visited flag across every section.
  int stamp = -1;
  // Face the BFS entered through, or -1 for the root. Only faces reachable
  // from it are followed onwards.
  int entryFace = -1;
  [[nodiscard]] bool visitedIn(int walkStamp) const noexcept {
   return stamp == walkStamp;
  }
  void enter(int walkStamp, int face) noexcept {
   stamp = walkStamp;
   entryFace = face;
  }
  // Can the walk pass from the face it came in through, out of `face`?
  // An unbuilt section has no mesh to occlude with, so it stays transparent.
  [[nodiscard]] bool connects(int face, bool built) const noexcept {
   if(entryFace < 0 || !built) {
    return true;
   }
   return (visBits & (1ULL << (entryFace * 6 + face))) != 0;
  }
 };
 OcclusionState occlusion{};
 std::vector<::net::minecraft::block::entity::BlockEntity*> blockEntities_{};
 std::vector<::net::minecraft::block::entity::BlockEntity*>* currentBlockEntities_ = nullptr;
};
} // namespace net::minecraft::client::render::chunk
