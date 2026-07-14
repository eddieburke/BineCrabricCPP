#include "net/minecraft/client/render/lod/LodMesher.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include <algorithm>
namespace net::minecraft::client::render::lod {
namespace {
struct CellSample {
  int top = 0;
  int ground = 0;
  std::uint8_t block = 0;
  bool water = false;
  bool valid = false;
};
struct SampleGrid {
  int cells = 0;
  std::vector<CellSample> samples;
  [[nodiscard]] const CellSample& at(int cellX, int cellZ) const noexcept {
    return samples[static_cast<std::size_t>((cellZ + 1) * (cells + 2) + (cellX + 1))];
  }
  [[nodiscard]] CellSample& at(int cellX, int cellZ) noexcept {
    return samples[static_cast<std::size_t>((cellZ + 1) * (cells + 2) + (cellX + 1))];
  }
};
constexpr int kSnapshotSpan = kRegionChunks + 2;
[[nodiscard]] const LodChunk* snapshotChunk(const LodMeshJob& job, int chunkX, int chunkZ) noexcept {
  if(chunkX < -1 || chunkX > kRegionChunks || chunkZ < -1 || chunkZ > kRegionChunks) {
    return nullptr;
  }
  const std::size_t index = static_cast<std::size_t>((chunkZ + 1) * kSnapshotSpan + (chunkX + 1));
  if(index >= job.chunks.size() || job.present[index] == 0) {
    return nullptr;
  }
  return &job.chunks[index];
}
[[nodiscard]] CellSample sampleCell(const LodMeshJob& job, int cellX, int cellZ, int cellSize) noexcept {
  CellSample best{};
  const int baseX = cellX * cellSize;
  const int baseZ = cellZ * cellSize;
  for(int dz = 0; dz < cellSize; ++dz) {
    const int blockZ = baseZ + dz;
    const int chunkZ = blockZ >> 4;
    for(int dx = 0; dx < cellSize; ++dx) {
      const int blockX = baseX + dx;
      const int chunkX = blockX >> 4;
      const LodChunk* chunk = snapshotChunk(job, chunkX, chunkZ);
      if(chunk == nullptr) {
        continue;
      }
      const LodColumn& column = chunk->at(blockX & 15, blockZ & 15);
      if(!column.hasSurface()) {
        continue;
      }
      const int top = column.effectiveTop();
      if(!best.valid || top > best.top) {
        best.top = top;
        best.ground = column.topY;
        best.block = column.topBlock;
        best.water = column.waterY > column.topY;
        best.valid = true;
      }
    }
  }
  return best;
}
[[nodiscard]] std::uint32_t shade(std::uint32_t rgba, int numerator, int denominator) noexcept {
  const std::uint32_t r = (rgba & 0xFFU) * static_cast<std::uint32_t>(numerator) / static_cast<std::uint32_t>(denominator);
  const std::uint32_t g =
      ((rgba >> 8) & 0xFFU) * static_cast<std::uint32_t>(numerator) / static_cast<std::uint32_t>(denominator);
  const std::uint32_t b =
      ((rgba >> 16) & 0xFFU) * static_cast<std::uint32_t>(numerator) / static_cast<std::uint32_t>(denominator);
  return r | (g << 8) | (b << 16) | 0xFF000000U;
}
void emitQuad(std::vector<TessellatorVertex>& out,
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
  TessellatorVertex v0;
  v0.u = 0.5f; v0.v = 0.5f; v0.color = color; v0.x = x0; v0.y = y0; v0.z = z0;
  TessellatorVertex v1;
  v1.u = 0.5f; v1.v = 0.5f; v1.color = color; v1.x = x1; v1.y = y1; v1.z = z1;
  TessellatorVertex v2;
  v2.u = 0.5f; v2.v = 0.5f; v2.color = color; v2.x = x2; v2.y = y2; v2.z = z2;
  TessellatorVertex v3;
  v3.u = 0.5f; v3.v = 0.5f; v3.color = color; v3.x = x3; v3.y = y3; v3.z = z3;

  if(Tessellator::effectiveDrawMode(7) != 7) {
    out.push_back(v0);
    out.push_back(v1);
    out.push_back(v2);
    out.push_back(v0);
    out.push_back(v2);
    out.push_back(v3);
  } else {
    out.push_back(v0);
    out.push_back(v1);
    out.push_back(v2);
    out.push_back(v3);
  }
}
constexpr std::uint32_t kFallbackColor = 0xFF7F7F7FU;
constexpr std::uint32_t kWaterColor = 0xFFC84D2CU;
} // namespace
void LodMeshJob::build(LodMeshJob& job) {
  job.vertices.clear();
  const auto* colors = job.colors[0];
  const auto* sideColors = job.colors[1];
  const int cellSize = 1 << job.level;
  const int cells = kRegionBlocks / cellSize;
  SampleGrid grid;
  grid.cells = cells;
  grid.samples.assign(static_cast<std::size_t>((cells + 2) * (cells + 2)), CellSample{});
  for(int cellZ = -1; cellZ <= cells; ++cellZ) {
    for(int cellX = -1; cellX <= cells; ++cellX) {
      grid.at(cellX, cellZ) = sampleCell(job, cellX, cellZ, cellSize);
    }
  }
  float minY = 130.0f;
  float maxY = 0.0f;
  const float s = static_cast<float>(cellSize);
  for(int cellZ = 0; cellZ < cells; ++cellZ) {
    for(int cellX = 0; cellX < cells; ++cellX) {
      const CellSample& cell = grid.at(cellX, cellZ);
      if(!cell.valid) {
        continue;
      }
      std::uint32_t topColor = kFallbackColor;
      if(cell.water) {
        topColor = kWaterColor;
      } else if(colors != nullptr && (*colors)[cell.block] != 0) {
        topColor = (*colors)[cell.block];
      }
      std::uint32_t sideBase = topColor;
      if(!cell.water && sideColors != nullptr && (*sideColors)[cell.block] != 0) {
        sideBase = (*sideColors)[cell.block];
      }
      const float x0 = job.originX + static_cast<float>(cellX) * s;
      const float z0 = job.originZ + static_cast<float>(cellZ) * s;
      const float x1 = x0 + s;
      const float z1 = z0 + s;
      const float top = static_cast<float>(cell.top + 1);
      int occlusion = 0;
      const auto raised = [&](int nx, int nz) {
        const CellSample& neighbor = grid.at(nx, nz);
        if(neighbor.valid && neighbor.top > cell.top) {
          ++occlusion;
        }
      };
      raised(cellX - 1, cellZ);
      raised(cellX + 1, cellZ);
      raised(cellX, cellZ - 1);
      raised(cellX, cellZ + 1);
      const std::uint32_t topShaded = occlusion > 0 ? shade(topColor, 16 - occlusion, 16) : topColor;
      emitQuad(job.vertices, x0, top, z0, x0, top, z1, x1, top, z1, x1, top, z0, topShaded);
      minY = std::min(minY, top);
      maxY = std::max(maxY, top);
      const auto wall = [&](const CellSample& neighbor,
                            float ax,
                            float az,
                            float bx,
                            float bz,
                            std::uint32_t sideColor) {
        int neighborTop = cell.top;
        if(neighbor.valid) {
          neighborTop = neighbor.top;
        }
        if(neighborTop >= cell.top) {
          return;
        }
        const float bottom = static_cast<float>(neighborTop + 1);
        emitQuad(job.vertices, ax, top, az, bx, top, bz, bx, bottom, bz, ax, bottom, az, sideColor);
        minY = std::min(minY, bottom);
      };
      const std::uint32_t xShade = shade(sideBase, 3, 5);
      const std::uint32_t zShade = shade(sideBase, 4, 5);
      wall(grid.at(cellX - 1, cellZ), x0, z0, x0, z1, xShade);
      wall(grid.at(cellX + 1, cellZ), x1, z0, x1, z1, xShade);
      wall(grid.at(cellX, cellZ - 1), x0, z0, x1, z0, zShade);
      wall(grid.at(cellX, cellZ + 1), x0, z1, x1, z1, zShade);
    }
  }
  if(job.vertices.empty()) {
    minY = 0.0f;
    maxY = 0.0f;
  }
  job.minY = minY;
  job.maxY = maxY;
}
} // namespace net::minecraft::client::render::lod
