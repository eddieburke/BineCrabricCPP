#pragma once
#include <cstddef>
#include <vector>
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::render::chunk {
// One persistent GL vertex buffer holding the geometry of every resident render
// section that shares a camera-offset group, for a single render layer. Each
// section owns a contiguous sub-range (a Slot) inside the buffer; sections are
// uploaded once per rebuild via bufferSubData and drawn together every frame
// through one shared interleaved vertex layout.
//
// A CPU shadow of the buffer is kept so the buffer can be grown (reallocated)
// without re-tessellating: on growth we just re-upload the shadow. The shadow
// doubles vertex RAM versus the old discard-after-compile path; that is the
// deliberate trade for safe, stall-free growth and can later be replaced with a
// GPU-to-GPU copy.
//
// All methods touch GL and must run on the render (main) thread.
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
 // Return a slot's range to the free list. Leaves slot invalid.
 void release(Slot& slot) noexcept;
 // A contiguous run of vertices to draw, in vertices. Adjacent sections often
 // occupy adjacent slots, so flush() coalesces these before drawing.
 struct DrawRange {
  int first = 0;
  int count = 0;
 };
 // Per-frame visible-range collection.
 void beginFrame() noexcept;
 void addVisible(const Slot& slot);
 [[nodiscard]] bool hasVisible() const noexcept {
  return !visible_.empty();
 }
 // Bind the buffer, configure the shared vertex layout, and draw the visible
 // slots as coalesced ranges. The caller owns the modelview transform (region
 // origin translate). Returns the number of draw calls actually submitted.
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
 std::vector<DrawRange> merged_; // per-frame, sorted by offset and coalesced
};
} // namespace net::minecraft::client::render::chunk
