#pragma once
#include <array>
#include <cstddef>
#include <map>
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
std::span<const std::byte> packTerrainVertices(std::span<TessellatorVertex> vertices) noexcept;
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
 bool upload(int layer, TerrainAllocation& allocation, std::span<TessellatorVertex> vertices);
 void release(int layer, TerrainAllocation& allocation) noexcept;
 int drawLayer(int layer, std::span<const TerrainAllocation* const> allocations);
 [[nodiscard]] std::size_t gpuBytes() const noexcept;
 [[nodiscard]] std::size_t liveBytes() const noexcept;
 std::array<std::size_t, terrain_layer::Count> drawListIndices{};
 static constexpr std::size_t kGranuleVertices = 64;

 private:
 struct LayerArena {
  unsigned handle = 0;
  unsigned vao = 0;
  std::size_t capacityVertices = 0;
  std::size_t liveVertices = 0;
  std::map<std::size_t, std::size_t> freeRanges{};
  std::vector<int> indexCounts{};
  std::vector<int> baseVertices{};
 };
 bool rebuild(int layer, std::size_t capacityVertices);
 bool acquire(int layer, std::size_t vertices, TerrainAllocation& allocation);
 static void releaseRange(LayerArena& arena, TerrainAllocation& allocation) noexcept;
 void shrinkIfSparse(int layer);
 int originX_ = 0;
 int originY_ = 0;
 int originZ_ = 0;
 std::array<LayerArena, terrain_layer::Count> layers_{};
 std::vector<ChunkBuilder*> sections_{};
};
} // namespace net::minecraft::client::render::chunk
