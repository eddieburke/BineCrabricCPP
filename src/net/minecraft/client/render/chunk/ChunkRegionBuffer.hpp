#pragma once
#include <array>
#include <cstddef>
#include <memory>
#include <vector>
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
namespace net::minecraft::client::render::chunk {
class ChunkRegionBuffer {
 public:
 // A section's allocation within the buffer. offset/capacity/count are in
 // vertices. capacity >= count so a slightly smaller rebuild reuses the slot.
 struct Slot {
  int offset = 0;
  int capacity = 0;
  int count = 0;
  [[nodiscard]] bool valid() const noexcept {
   return capacity > 0;
  }
 };
 ChunkRegionBuffer() = default;
 ~ChunkRegionBuffer();
 ChunkRegionBuffer(const ChunkRegionBuffer&) = delete;
 ChunkRegionBuffer& operator=(const ChunkRegionBuffer&) = delete;
 ChunkRegionBuffer(ChunkRegionBuffer&&) = delete;
 ChunkRegionBuffer& operator=(ChunkRegionBuffer&&) = delete;
  // Write a section's vertices into slot, allocating or reallocating as needed.
  // The first call should pass an empty slot. Attribute layout flags for the
  // whole buffer are taken from the first non-empty upload.
  void upload(Slot& slot, const TessellatorVertex* data, int count, bool hasTexture, bool hasColor, bool hasNormals);
  // Pre-allocate GPU storage (and the free/tail bookkeeping) so the initial world
  // load does not repeatedly grow the buffer and re-upload every accumulated
  // vertex range. Calling it after uploads is harmless — it only ever grows.
  void reserve(std::size_t vertexCapacity);
  // Return a slot's range to the free list. Leaves slot invalid.
  void release(Slot& slot) noexcept;
 struct DrawRange {
  int first = 0;
  int count = 0;
  float chunkOffset[3] = {0.0f, 0.0f, 0.0f};
 };
 // Per-frame visible-range collection.
 void beginFrame() noexcept;
 void addVisible(const Slot& slot, float chunkOffsetX, float chunkOffsetY, float chunkOffsetZ);
 [[nodiscard]] bool hasVisible() const noexcept {
  return !visible_.empty();
 }
 // Bind the buffer, configure the shared vertex layout, and draw each visible
 // slot with its chunkOffset. Ranges that share the exact same chunkOffset and
 // are adjacent in the VBO may coalesce. Returns draw calls submitted.
 int flush();
 [[nodiscard]] bool empty() const noexcept {
  return shadow_.empty();
 }
 // Per-frame draw-call accounting for the debug HUD. Reset by WorldRenderer at
 // the start of each frame's draw-list build; accumulated across all regions.
 inline static int frameVisibleRanges = 0;
 inline static int frameDrawCalls = 0;

 private:
 void reallocBuffer(std::size_t newCapacityVertices);
 void buildMergedRanges();
 [[nodiscard]] Slot allocate(int count);
 unsigned int handle_ = 0; // GL buffer name; 0 until first flush
 std::size_t gpuCapacity_ = 0; // vertices the GL buffer can hold
 std::vector<TessellatorVertex> shadow_; // CPU mirror, size == used tail
 // Free ranges below the tail, kept sorted by offset and coalesced.
 std::vector<Slot> freeList_;
 bool hasTexture_ = false;
 bool hasColor_ = false;
 bool hasNormals_ = false;
 bool layoutSet_ = false;
 std::vector<DrawRange> visible_; // per-frame, in insertion (ring) order
 std::vector<DrawRange> lastVisible_;
 std::vector<DrawRange> merged_; // per-frame, sorted by offset and coalesced
};

struct ChunkRegion {
 std::array<ChunkRegionBuffer, terrain_layer::Count> layers{};
};
class ChunkRegionManager {
 public:
  ChunkRegion& pool() noexcept {
   return region_;
  }
  void clear() noexcept {}

 private:
  ChunkRegion region_;
};
} // namespace net::minecraft::client::render::chunk
