#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
namespace net::minecraft::client::render::chunk {
// Sections are fixed 16^3 blocks. Regions are 8x4x8 sections (128x64x128 blocks):
// Sodium's granularity. A section at section coords (sx, sy, sz) lives in region
// (sx>>3, sy>>2, sz>>3).
inline constexpr int kSectionBlocks = 16;
inline constexpr int kRegionSectionsX = 8;
inline constexpr int kRegionSectionsY = 4;
inline constexpr int kRegionSectionsZ = 8;
inline constexpr int kRegionBlocksX = kRegionSectionsX * 16;
inline constexpr int kRegionBlocksY = kRegionSectionsY * 16;
inline constexpr int kRegionBlocksZ = kRegionSectionsZ * 16;
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
   // whole buffer are taken from the first non-empty upload. bake* is added to
   // every position: sections arrive section-local, and terrain is stored
   // region-local so the whole region shares one chunkOffset at draw time.
   void upload(Slot& slot,
               const TessellatorVertex* data,
               int count,
               bool hasTexture,
               bool hasColor,
               bool hasNormals,
               float bakeX,
               float bakeY,
               float bakeZ);
  // Pre-allocate GPU storage (and the free/tail bookkeeping) so the initial world
  // load does not repeatedly grow the buffer and re-upload every accumulated
  // vertex range. Calling it after uploads is harmless — it only ever grows.
  void reserve(std::size_t vertexCapacity);
  // Return a slot's range to the free list. Leaves slot invalid.
  void release(Slot& slot) noexcept;
  struct DrawRange {
   int first = 0;
   int count = 0;
  };
  // Per-frame visible-range collection. beginFrame takes the region's chunk
  // offset (region origin - camera), shared by every range of this buffer.
  void beginFrame(float chunkOffsetX, float chunkOffsetY, float chunkOffsetZ) noexcept;
  void addVisible(const Slot& slot);
  // Bind the buffer, configure the shared vertex layout, and draw every visible
  // range in one call with the frame's chunkOffset. Returns draw calls submitted.
  int flush();
  // Per-frame draw-call accounting for the debug HUD. Reset by WorldRenderer at
  // the start of each frame's draw-list build; accumulated across all regions.
  inline static int frameVisibleRanges = 0;
  inline static int frameDrawCalls = 0;
  // TEMP DIAGNOSTIC — [terrain-probe]. When enabled, flush() accumulates the
  // camera-relative AABB the drawn ranges actually reconstruct to
  // (vaPosition + chunkOffset), plus the matrices it uploaded. WorldRenderer
  // arms it once every N frames and logs the result. This answers, in one run,
  // whether terrain geometry is misplaced or the matrices are.
  inline static bool probeArmed = false;
  inline static int probeRegions = 0;
  inline static int probeRanges = 0;
  inline static long long probeVertices = 0;
  inline static float probeMin[3] = {0.0f, 0.0f, 0.0f};
  inline static float probeMax[3] = {0.0f, 0.0f, 0.0f};
  inline static bool probeAny = false;
  inline static float probeProjection[16] = {0.0f};
  inline static float probeModelView[16] = {0.0f};

 private:
  void reallocBuffer(std::size_t newCapacityVertices);
  void buildMergedRanges();
  [[nodiscard]] Slot allocate(int count);
  unsigned int handle_ = 0; // GL buffer name; 0 until first flush
  std::size_t gpuCapacity_ = 0; // vertices the GL buffer can hold
  std::vector<TessellatorVertex> shadow_; // CPU mirror, size == used tail
  // Free ranges below the tail, kept sorted by offset and coalesced.
  std::vector<Slot> freeList_;
  float frameChunkOffset_[3] = {0.0f, 0.0f, 0.0f};
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
 int sectionCount = 0;
};
struct RegionKey {
 int x = 0;
 int y = 0;
 int z = 0;
 [[nodiscard]] bool operator==(const RegionKey& other) const noexcept {
  return x == other.x && y == other.y && z == other.z;
 }
};
struct RegionKeyHash {
 [[nodiscard]] std::size_t operator()(const RegionKey& key) const noexcept {
  std::size_t h = static_cast<std::uint32_t>(key.x) * 0x9E3779B1u;
  h ^= static_cast<std::uint32_t>(key.z) * 0x85EBCA77u + (h << 6) + (h >> 2);
  h ^= static_cast<std::uint32_t>(key.y) * 0xC2B2AE3Du + (h << 6) + (h >> 2);
  return h;
 }
};
class ChunkRegionManager {
 public:
  // Look up or create the region containing section (sectionX, sectionY,
  // sectionZ). Created regions pre-reserve setReserveHint capacity per layer.
  ChunkRegion& regionFor(int sectionX, int sectionY, int sectionZ);
  // Drops the region when its last section has been released.
  void releaseRegion(const RegionKey& key);
  void clear() noexcept;
  void setReserveHint(std::size_t perRegionVertices) noexcept {
   reserveHint_ = perRegionVertices;
  }
  template <typename Fn>
  void forEachRegion(Fn&& fn) {
   for(auto& entry : regions_) fn(entry.first, *entry.second);
  }
  [[nodiscard]] std::size_t regionCount() const noexcept {
   return regions_.size();
  }

 private:
  std::unordered_map<RegionKey, std::unique_ptr<ChunkRegion>, RegionKeyHash> regions_{};
  std::size_t reserveHint_ = 0;
};
} // namespace net::minecraft::client::render::chunk
