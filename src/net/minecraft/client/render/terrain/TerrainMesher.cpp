#include "net/minecraft/client/render/terrain/TerrainMesher.hpp"
#include <algorithm>
#include "net/minecraft/client/render/terrain/TerrainSurfaceData.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::terrain {
namespace {
constexpr int kSnapshotSpan = kTerrainRegionChunks + 2;
constexpr std::uint32_t kFallbackColor = 0xFF7F7F7FU;
struct Column {
  int topY = 0;
  int topBlock = 0;
  int groundBlock = 0;
  int waterY = 0;
  bool valid = false;
};
struct Cell {
  int top = 0;
  int ground = 0;
  int block = 0;
  int groundBlock = 0;
  bool water = false;
  bool valid = false;
};
struct Grid {
  int cells = 0;
  std::vector<Cell> values{};
  [[nodiscard]] Cell& at(int x, int z) noexcept {
    return values[static_cast<std::size_t>((z + 1) * (cells + 2) + x + 1)];
  }
  [[nodiscard]] const Cell& at(int x, int z) const noexcept {
    return values[static_cast<std::size_t>((z + 1) * (cells + 2) + x + 1)];
  }
};
[[nodiscard]] Column columnAt(std::span<const std::uint8_t> snapshot, int blockX, int blockZ) noexcept {
  const int chunkX = MathHelper::floorDiv(blockX, 16);
  const int chunkZ = MathHelper::floorDiv(blockZ, 16);
  if(chunkX < -1 || chunkX > kTerrainRegionChunks || chunkZ < -1 || chunkZ > kTerrainRegionChunks) {
    return {};
  }
  const int localX = blockX - chunkX * 16;
  const int localZ = blockZ - chunkZ * 16;
  const std::size_t chunkIndex = static_cast<std::size_t>((chunkZ + 1) * kSnapshotSpan + chunkX + 1);
  const std::size_t base = chunkIndex * static_cast<std::size_t>(kTerrainChunkBytes);
  if(base >= snapshot.size() || snapshot[base] == 0) {
    return {};
  }
  const std::size_t offset = base + 1 + static_cast<std::size_t>(localZ * 16 + localX) * kTerrainColumnBytes;
  if(offset + 3 >= snapshot.size()) {
    return {};
  }
  Column column{
      snapshot[offset], snapshot[offset + 1], snapshot[offset + 2], snapshot[offset + 3], true};
  if(column.topBlock == 0 && column.waterY == 0) {
    column.valid = false;
  }
  return column;
}
[[nodiscard]] Cell sampleCell(std::span<const std::uint8_t> snapshot,
                              int cellX,
                              int cellZ,
                              int cellSize) noexcept {
  const int baseX = cellX * cellSize;
  const int baseZ = cellZ * cellSize;
  int ground = -1;
  int waterTop = -1;
  int block = 0;
  int groundBlock = 0;
  int waterColumns = 0;
  int landColumns = 0;
  for(int dz = 0; dz < cellSize; ++dz) {
    for(int dx = 0; dx < cellSize; ++dx) {
      const Column column = columnAt(snapshot, baseX + dx, baseZ + dz);
      if(!column.valid) {
        continue;
      }
      if(column.topY > ground) {
        ground = column.topY;
        block = column.topBlock;
        groundBlock = column.groundBlock;
      }
      if(column.waterY > column.topY) {
        ++waterColumns;
        waterTop = std::max(waterTop, column.waterY);
      } else {
        ++landColumns;
      }
    }
  }
  if(ground < 0 && waterTop < 0) {
    return {};
  }
  const bool water = waterColumns > landColumns && waterTop > ground;
  return {
      water ? waterTop : ground,
      std::max(ground, 0),
      block,
      groundBlock != 0 ? groundBlock : block,
      water,
      true,
  };
}
[[nodiscard]] std::uint32_t shade(std::uint32_t color, int numerator, int denominator) noexcept {
  const std::uint32_t r = (color & 0xFFU) * static_cast<std::uint32_t>(numerator) /
                          static_cast<std::uint32_t>(denominator);
  const std::uint32_t g = ((color >> 8U) & 0xFFU) * static_cast<std::uint32_t>(numerator) /
                          static_cast<std::uint32_t>(denominator);
  const std::uint32_t b = ((color >> 16U) & 0xFFU) * static_cast<std::uint32_t>(numerator) /
                          static_cast<std::uint32_t>(denominator);
  return r | (g << 8U) | (b << 16U) | 0xFF000000U;
}
void emitVertex(std::vector<TessellatorVertex>& vertices, float x, float y, float z, std::uint32_t color) {
  TessellatorVertex vertex;
  vertex.x = x;
  vertex.y = y;
  vertex.z = z;
  vertex.color = color;
  vertices.push_back(vertex);
}
void emitQuad(std::vector<TessellatorVertex>& vertices,
              float x0,
              float y0,
              float z0,
              float x1,
              float y1,
              float z1,
              float x2,
              float y2,
              float z2,
              float x3,
              float y3,
              float z3,
              std::uint32_t color) {
  emitVertex(vertices, x0, y0, z0, color);
  emitVertex(vertices, x1, y1, z1, color);
  emitVertex(vertices, x2, y2, z2, color);
  emitVertex(vertices, x0, y0, z0, color);
  emitVertex(vertices, x2, y2, z2, color);
  emitVertex(vertices, x3, y3, z3, color);
}
} // namespace
void buildTerrainMesh(std::span<const std::uint8_t> snapshot,
                      int level,
                      const std::array<std::uint32_t, 256>& topColors,
                      const std::array<std::uint32_t, 256>& sideColors,
                      TerrainMeshResult& result) {
  result.vertices.clear();
  result.minY = 0.0f;
  result.maxY = 0.0f;
  level = std::clamp(level, 0, 4);
  const int cellSize = 1 << level;
  const int cells = kTerrainRegionBlocks / cellSize;
  Grid grid;
  grid.cells = cells;
  grid.values.resize(static_cast<std::size_t>((cells + 2) * (cells + 2)));
  for(int z = -1; z <= cells; ++z) {
    for(int x = -1; x <= cells; ++x) {
      grid.at(x, z) = sampleCell(snapshot, x, z, cellSize);
    }
  }
  result.minY = 130.0f;
  result.maxY = 0.0f;
  const float size = static_cast<float>(cellSize);
  for(int z = 0; z < cells; ++z) {
    for(int x = 0; x < cells; ++x) {
      const Cell& cell = grid.at(x, z);
      if(!cell.valid) {
        continue;
      }
      const int colorBlock = cell.water ? 8 : cell.block;
      std::uint32_t topColor = topColors[static_cast<std::size_t>(colorBlock)];
      if(topColor == 0) {
        topColor = kFallbackColor;
      }
      std::uint32_t sideColor = sideColors[static_cast<std::size_t>(colorBlock)];
      if(sideColor == 0) {
        sideColor = topColor;
      }
      std::uint32_t cliffColor = sideColors[static_cast<std::size_t>(cell.groundBlock)];
      if(cliffColor == 0) {
        cliffColor = sideColor;
      }
      const float x0 = static_cast<float>(x) * size;
      const float z0 = static_cast<float>(z) * size;
      const float x1 = x0 + size;
      const float z1 = z0 + size;
      const float top = static_cast<float>(cell.top + 1);
      int occlusion = 0;
      const auto raised = [&](int nx, int nz) {
        const Cell& neighbor = grid.at(nx, nz);
        if(!neighbor.valid) {
          return;
        }
        const int neighborHeight = neighbor.water ? neighbor.top : neighbor.ground;
        const int cellHeight = cell.water ? cell.top : cell.ground;
        if(neighborHeight > cellHeight) {
          ++occlusion;
        }
      };
      raised(x - 1, z);
      raised(x + 1, z);
      raised(x, z - 1);
      raised(x, z + 1);
      const std::uint32_t topShaded = occlusion > 0 ? shade(topColor, 16 - occlusion, 16) : topColor;
      emitQuad(result.vertices, x0, top, z0, x0, top, z1, x1, top, z1, x1, top, z0, topShaded);
      result.minY = std::min(result.minY, top);
      result.maxY = std::max(result.maxY, top);
      const auto wall = [&](int nx, int nz, float ax, float az, float bx, float bz, std::uint32_t color) {
        const Cell& neighbor = grid.at(nx, nz);
        if(!neighbor.valid) {
          return;
        }
        const int wallTop = cell.water ? cell.top : cell.ground + 1;
        const int wallBottom = neighbor.water ? neighbor.top : neighbor.ground + 1;
        if(wallTop <= wallBottom) {
          return;
        }
        const float topY = static_cast<float>(wallTop);
        const float bottomY = static_cast<float>(wallBottom);
        emitQuad(result.vertices, ax, topY, az, bx, topY, bz, bx, bottomY, bz, ax, bottomY, az, color);
        result.minY = std::min(result.minY, bottomY);
      };
      wall(x - 1, z, x0, z0, x0, z1, shade(cliffColor, 3, 5));
      wall(x + 1, z, x1, z0, x1, z1, shade(cliffColor, 3, 5));
      wall(x, z - 1, x0, z0, x1, z0, shade(cliffColor, 4, 5));
      wall(x, z + 1, x0, z1, x1, z1, shade(cliffColor, 4, 5));
    }
  }
  if(result.vertices.empty()) {
    result.minY = 0.0f;
    result.maxY = 0.0f;
  }
}
} // namespace net::minecraft::client::render::terrain
