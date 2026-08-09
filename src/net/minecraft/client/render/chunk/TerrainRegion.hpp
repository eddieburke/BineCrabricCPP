#pragma once
#include <array>
#include <cstddef>
#include <span>
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
 struct Range {
  std::size_t first = 0;
  std::size_t capacity = 0;
 };
 struct LayerArena {
  unsigned handle = 0;
  unsigned vao = 0;
  std::size_t capacityVertices = 0;
  std::size_t tailVertex = 0;
  std::vector<Range> freeRanges{};
  std::vector<int> indexCounts{};
  std::vector<int> baseVertices{};
 };
 bool ensureCapacity(LayerArena& arena, std::size_t requiredVertices);
 bool acquire(LayerArena& arena, std::size_t requiredVertices, TerrainAllocation& allocation);
 static void releaseRange(LayerArena& arena, TerrainAllocation& allocation) noexcept;
 int originX_ = 0;
 int originY_ = 0;
 int originZ_ = 0;
 std::array<LayerArena, terrain_layer::Count> layers_{};
 std::vector<ChunkBuilder*> sections_{};
};
}
