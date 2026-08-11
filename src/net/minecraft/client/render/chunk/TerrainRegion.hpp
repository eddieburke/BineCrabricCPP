#pragma once
#include <array>
#include <cstddef>
#include <span>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
namespace net::minecraft::client::render::chunk {
class ChunkBuilder;
struct TerrainAllocation {
 std::size_t firstVertex = 0;
 std::size_t capacityVertices = 0;
 int vertexCount = 0;
 [[nodiscard]] bool valid() const noexcept {
  return capacityVertices != 0 && vertexCount != 0;
 }
};
class TerrainRegion {
 public:
 TerrainRegion(int originX, int originY, int originZ);
 ~TerrainRegion();
 TerrainRegion(const TerrainRegion&) = delete;
 TerrainRegion& operator=(const TerrainRegion&) = delete;
 [[nodiscard]] int originX() const noexcept { return originX_; }
 [[nodiscard]] int originY() const noexcept { return originY_; }
 [[nodiscard]] int originZ() const noexcept { return originZ_; }
 [[nodiscard]] const std::vector<ChunkBuilder*>& sections() const noexcept { return sections_; }
 void addSection(ChunkBuilder* section);
 void removeSection(ChunkBuilder* section);
 bool upload(int layer, TerrainAllocation& allocation, std::span<const TessellatorVertex> vertices);
 void release(int layer, TerrainAllocation& allocation) noexcept;
 int drawLayer(int layer, std::span<const TerrainAllocation* const> allocations);

 private:
 static constexpr int kMinSizeClass = 8;
 static constexpr int kMaxSizeClass = 31;
 static constexpr int kSizeClasses = kMaxSizeClass - kMinSizeClass + 1;
 struct LayerArena {
  unsigned handle = 0;
  unsigned vao = 0;
  unsigned staging = 0;
  std::size_t capacityVertices = 0;
  std::size_t tailVertex = 0;
  std::array<std::unordered_set<std::size_t>, kSizeClasses> freeByClass{};
  std::vector<int> indexCounts{};
  std::vector<int> baseVertices{};
 };
 [[nodiscard]] static std::unordered_set<std::size_t>& freeList(LayerArena& arena, int sizeClass) noexcept {
  return arena.freeByClass[static_cast<std::size_t>(sizeClass - kMinSizeClass)];
 }
 static void trimTail(LayerArena& arena) noexcept;
 bool ensureCapacity(LayerArena& arena, std::size_t requiredVertices);
 bool acquire(LayerArena& arena, std::size_t requiredVertices, TerrainAllocation& allocation);
 static void releaseRange(LayerArena& arena, TerrainAllocation& allocation) noexcept;
 int originX_ = 0;
 int originY_ = 0;
 int originZ_ = 0;
 std::array<LayerArena, terrain_layer::Count> layers_{};
 std::vector<ChunkBuilder*> sections_{};
};
} // namespace net::minecraft::client::render::chunk
