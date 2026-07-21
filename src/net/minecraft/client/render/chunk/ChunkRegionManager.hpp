#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include "net/minecraft/client/render/chunk/ChunkRegionBuffer.hpp"
namespace net::minecraft::client::render::chunk {
// A camera-offset group: every resident section whose cameraOffsetX/Z match
// shares one ChunkRegion, which owns a vertex buffer per render layer. Because
// section vertices are baked into region space at upload, the whole region draws
// with a single outer translate and one glMultiDrawArrays per layer.
struct ChunkRegion {
 int offsetX = 0;
 int offsetY = 0;
 int offsetZ = 0;
 std::array<ChunkRegionBuffer, 2> layers{};
};
// Owns every ChunkRegion, keyed by packed (offsetX, offsetZ). offsetY is always
// 0 for cubic sections (renderY == y), so it is not part of the key, matching
// the old display-list grouping. All methods run on the render (main) thread.
class ChunkRegionManager {
 public:
 [[nodiscard]] static std::uint64_t keyFor(int offsetX, int offsetZ) noexcept {
  return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(offsetX)) << 32) |
         static_cast<std::uint64_t>(static_cast<std::uint32_t>(offsetZ));
 }
 // The region for a section's camera offset, created on first use.
 ChunkRegion& regionFor(int offsetX, int offsetY, int offsetZ) {
  const std::uint64_t key = keyFor(offsetX, offsetZ);
  std::unique_ptr<ChunkRegion>& slot = regions_[key];
  if(slot == nullptr) {
   slot = std::make_unique<ChunkRegion>();
   slot->offsetX = offsetX;
   slot->offsetY = offsetY;
   slot->offsetZ = offsetZ;
  }
  return *slot;
 }
 [[nodiscard]] auto begin() noexcept {
  return regions_.begin();
 }
 [[nodiscard]] auto end() noexcept {
  return regions_.end();
 }
 // Destroy every region buffer. GL buffers are freed by ~ChunkRegionBuffer, so
 // this must run with the GL context current.
 void clear() noexcept {
  regions_.clear();
 }

 private:
 std::unordered_map<std::uint64_t, std::unique_ptr<ChunkRegion>> regions_;
};
} // namespace net::minecraft::client::render::chunk
