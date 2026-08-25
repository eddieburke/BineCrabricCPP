// Terrain capture + mesh throughput over a fully loaded chunk grid.
//
// The CPU shape of extreme render scale: scale itself is a GPU multiplier, but what
// it costs the CPU is sections entering the compile queue when the camera turns. So
// this rebuilds every section of the grid and times capture and mesh separately.
//
// The contended run is the look-around hitch: light workers writing cells into the
// chunks being captured. LockedCaptures must stay at zero -- if it moves, the capture
// is blocking on the chunk write lock again. See
// docs/handoff-vtune-lookaround-hitch.md.
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <thread>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
namespace net::minecraft::test {
namespace {
using client::render::Tessellator;
using client::render::chunk::RegionSnapshot;
namespace terrain_layer = client::render::chunk::terrain_layer;
constexpr int kSection = 16;
// 20x20 chunks is ~33 MB of block/light arrays and, at 8 sections per column,
// 2,592 interior sections -- a full world rebuild, which is what a render-scale
// or shaderpack change actually triggers.
constexpr int kGrid = 20;
constexpr int kColumnSections = 8; // y in [0,128)
[[nodiscard]] std::uint32_t hash2(int x, int z) {
 std::uint32_t h = static_cast<std::uint32_t>(x) * 374761393U + static_cast<std::uint32_t>(z) * 668265263U;
 h = (h ^ (h >> 13)) * 1274126177U;
 return h ^ (h >> 16);
}
// A deterministic surface with hills, an ore-ish speckle and a water table, so the
// mesher emits real geometry in the solid, cutout and translucent layers rather
// than early-outing on empty sections.
void paintChunk(Chunk& chunk) {
 const auto stone = static_cast<std::uint8_t>(block::Block::STONE->id);
 const auto dirt = static_cast<std::uint8_t>(block::Block::DIRT->id);
 const auto grass = static_cast<std::uint8_t>(block::Block::GRASS_BLOCK->id);
 const auto water = static_cast<std::uint8_t>(block::Block::WATER->id);
 const auto glass = static_cast<std::uint8_t>(block::Block::GLASS->id);
 constexpr int kWaterLevel = 62;
 for(int localX = 0; localX < 16; ++localX) {
  for(int localZ = 0; localZ < 16; ++localZ) {
   const int worldX = chunk.x * 16 + localX;
   const int worldZ = chunk.z * 16 + localZ;
   const int height = 58 + static_cast<int>(hash2(worldX >> 2, worldZ >> 2) % 12U) +
                      static_cast<int>(hash2(worldX >> 5, worldZ >> 5) % 7U);
   const std::size_t column = static_cast<std::size_t>((localX << 11) | (localZ << 7));
   for(int y = 0; y < height - 4; ++y) {
    chunk.blocks[column + static_cast<std::size_t>(y)] = stone;
   }
   for(int y = height - 4; y < height; ++y) {
    chunk.blocks[column + static_cast<std::size_t>(y)] = dirt;
   }
   chunk.blocks[column + static_cast<std::size_t>(height)] = height < kWaterLevel ? dirt : grass;
   for(int y = height + 1; y <= kWaterLevel; ++y) {
    chunk.blocks[column + static_cast<std::size_t>(y)] = water;
   }
   // A scattering of glass: transparent, so it forces the mesher through the
   // neighbour-visibility path instead of culling whole faces away.
   if((hash2(worldX, worldZ) & 63U) == 0U) {
    chunk.blocks[column + static_cast<std::size_t>(height + 3)] = glass;
   }
   for(int y = 0; y < Chunk::height; ++y) {
    chunk.skyLight.set(localX, y, localZ, y > height ? 15 : (15 - (height - y)) & 0xF);
    chunk.blockLight.set(localX, y, localZ, (worldX * 3 + worldZ * 5 + y) & 0xF);
   }
  }
 }
 chunk.populateHeightMapOnly();
}
std::array<float, 16> linearLuminance() {
 std::array<float, 16> table{};
 for(int level = 0; level < 16; ++level) {
  table[static_cast<std::size_t>(level)] = Dimension::luminanceForLightLevel(level);
 }
 return table;
}
struct Grid {
 std::vector<std::unique_ptr<Chunk>> chunks;
 [[nodiscard]] Chunk* at(int chunkX, int chunkZ) const {
  if(chunkX < 0 || chunkZ < 0 || chunkX >= kGrid || chunkZ >= kGrid) {
   return nullptr;
  }
  return chunks[static_cast<std::size_t>(chunkX * kGrid + chunkZ)].get();
 }
};
Grid buildGrid() {
 Grid grid;
 grid.chunks.reserve(static_cast<std::size_t>(kGrid * kGrid));
 for(int chunkX = 0; chunkX < kGrid; ++chunkX) {
  for(int chunkZ = 0; chunkZ < kGrid; ++chunkZ) {
   auto chunk = std::make_unique<Chunk>(nullptr, chunkX, chunkZ);
   paintChunk(*chunk);
   grid.chunks.push_back(std::move(chunk));
  }
 }
 return grid;
}
struct PassResult {
 double captureMs = 0.0;
 double meshMs = 0.0;
 std::size_t sections = 0;
 std::size_t skippedSections = 0;
 std::size_t vertices = 0;
 std::uint64_t lockedCaptures = 0;
};
// Mirrors ChunkBuilder::createJob + buildMesh: same 3x3 chunk neighbourhood, same
// snapshot bounds, same per-layer scan. Mod meshes and block entities are left out
// so the measurement is the terrain path alone.
PassResult meshWholeGrid(const Grid& grid, bool skipEmptySections = false) {
 client::option::RenderSettings opts{};
 opts.ambientOcclusionActive = true;
 opts.ambientOcclusionStrength = 1.0f;
 opts.fancyLeaves = true;
 opts.fancyGrass = true;
 opts.renderWater = true;
 opts.fancyWater = true;
 PassResult result;
 const client::render::chunk::TerrainLayerTable layerOf = client::render::chunk::buildTerrainLayerTable({});
 const std::uint64_t lockedBefore = RegionSnapshot::lockedCaptureCount();
 static thread_local Tessellator tessellator;
 tessellator.setCaptureOnly(true);
 std::vector<RegionSnapshot::SourceChunk> sources;
 sources.reserve(9);
 for(int chunkX = 1; chunkX + 1 < kGrid; ++chunkX) {
  for(int chunkZ = 1; chunkZ + 1 < kGrid; ++chunkZ) {
   for(int sectionY = 0; sectionY < kColumnSections; ++sectionY) {
    if(skipEmptySections && !grid.at(chunkX, chunkZ)->sectionHasBlocks(sectionY)) {
     ++result.skippedSections;
     continue;
    }
    const int minX = chunkX * 16;
    const int minY = sectionY * kSection;
    const int minZ = chunkZ * 16;
    sources.clear();
    for(int neighbourX = chunkX - 1; neighbourX <= chunkX + 1; ++neighbourX) {
     for(int neighbourZ = chunkZ - 1; neighbourZ <= chunkZ + 1; ++neighbourZ) {
      sources.push_back(RegionSnapshot::SourceChunk{neighbourX, neighbourZ, grid.at(neighbourX, neighbourZ)});
     }
    }
    const auto captureStart = std::chrono::steady_clock::now();
    RegionSnapshot snapshot(sources,
                            /*ambientDarkness=*/0,
                            linearLuminance(),
                            /*biomeSource=*/nullptr,
                            minX - 1,
                            minY - 1,
                            minZ - 1,
                            minX + kSection + 1,
                            minY + kSection,
                            minZ + kSection + 1);
    const auto captureEnd = std::chrono::steady_clock::now();
    result.captureMs += std::chrono::duration<double, std::milli>(captureEnd - captureStart).count();
    ++result.sections;
    if(!snapshot.columnHasBlocks(minX, minZ, minY, minY + kSection)) {
     continue;
    }
    client::render::block::BlockRenderManager manager(tessellator, &snapshot, opts);
    for(int layer = 0; layer < terrain_layer::Count; ++layer) {
     const bool interiorFacePass = layer == terrain_layer::CutoutInterior;
     manager.ctx.interiorFacePass = interiorFacePass;
     bool began = false;
     for(int x = minX; x < minX + kSection; ++x) {
      for(int z = minZ; z < minZ + kSection; ++z) {
       const RegionSnapshot::ColumnNeighbourhood columns = snapshot.columnNeighbourhood(x, z);
       if(columns.self == nullptr) {
        continue;
       }
       for(int y = minY; y < minY + kSection; ++y) {
        const int row = columns.rowFor(y);
        const int blockId = columns.self[row];
        if(blockId <= 0) {
         continue;
        }
        if(columns.fullyEnclosed(row)) {
         continue;
        }
        block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
        if(block == nullptr) {
         continue;
        }
        if(interiorFacePass) {
         if(!client::render::block::hasLeafInteriorFaces(*block, opts.leafInteriorFaces)) {
          continue;
         }
        } else if(layerOf[static_cast<std::size_t>(blockId)] != layer) {
         continue;
        }
        if(!began) {
         began = true;
         tessellator.startQuads();
        }
        manager.render(*block, x, y, z);
       }
      }
     }
     if(began) {
      result.vertices += tessellator.takeMesh().vertexCount();
     }
    }
    const auto meshEnd = std::chrono::steady_clock::now();
    result.meshMs += std::chrono::duration<double, std::milli>(meshEnd - captureEnd).count();
   }
  }
 }
 result.lockedCaptures = RegionSnapshot::lockedCaptureCount() - lockedBefore;
 return result;
}
void report(const char* label, const PassResult& pass, std::uint64_t lightWrites) {
 const double total = pass.captureMs + pass.meshMs;
 std::printf(
     "[%-10s] %5zu sections  %8.1f ms total  capture %7.1f ms (%4.1f%%)  mesh %7.1f ms  "
     "%6.1f us/section  %.2f M verts  locked-captures %llu",
     label, pass.sections, total, pass.captureMs, 100.0 * pass.captureMs / (total > 0.0 ? total : 1.0),
     pass.meshMs, 1000.0 * total / static_cast<double>(pass.sections ? pass.sections : 1),
     static_cast<double>(pass.vertices) / 1e6, static_cast<unsigned long long>(pass.lockedCaptures));
 if(lightWrites > 0) {
  std::printf("  light-writes %.2f M", static_cast<double>(lightWrites) / 1e6);
 }
 std::printf("\n");
 std::fflush(stdout);
}
} // namespace
// Uncontended throughput: the cost of the capture+mesh path with nothing else
// touching the chunks. This is the number to watch when changing the mesher or
// the snapshot layout.
TEST(TerrainCapturePerf, QuietGridRebuild) {
 const Grid grid = buildGrid();
 const PassResult pass = meshWholeGrid(grid);
 report("quiet", pass, 0);
 EXPECT_GT(pass.vertices, 0u);
 EXPECT_EQ(pass.lockedCaptures, 0u);
}
TEST(TerrainCapturePerf, OccupancyIndexSkipsEmptyCaptures) {
 const Grid grid = buildGrid();
 const PassResult pass = meshWholeGrid(grid, true);
 report("occupied", pass, 0);
 EXPECT_GT(pass.vertices, 0u);
 EXPECT_GT(pass.skippedSections, 0u);
 EXPECT_EQ(pass.sections + pass.skippedSections,
           static_cast<std::size_t>((kGrid - 2) * (kGrid - 2) * kColumnSections));
}
// The look-around case: light workers writing cells into the chunks being
// captured. The capture must not block on them, and they must not block on it.
TEST(TerrainCapturePerf, GridRebuildUnderLightWriterContention) {
 const Grid grid = buildGrid();
 constexpr int kWriters = 4;
 std::atomic<bool> stop{false};
 std::atomic<std::uint64_t> writes{0};
 std::vector<std::thread> writers;
 writers.reserve(kWriters);
 for(int writer = 0; writer < kWriters; ++writer) {
  writers.emplace_back([&grid, &stop, &writes, writer] {
   std::uint64_t local = 0;
   std::uint32_t state = static_cast<std::uint32_t>(writer) * 2654435761U + 1U;
   while(!stop.load(std::memory_order_relaxed)) {
    for(int burst = 0; burst < 4096; ++burst) {
     state = state * 1664525U + 1013904223U;
     Chunk* chunk = grid.at(static_cast<int>((state >> 8) % kGrid), static_cast<int>((state >> 16) % kGrid));
     if(chunk == nullptr) {
      continue;
     }
     const int localX = static_cast<int>((state >> 4) & 15U);
     const int localZ = static_cast<int>((state >> 12) & 15U);
     const int y = static_cast<int>((state >> 20) & 127U);
     chunk->setLight(LightType::Block, localX, y, localZ, static_cast<int>(state & 15U));
     ++local;
    }
   }
   writes.fetch_add(local, std::memory_order_relaxed);
  });
 }
 const PassResult pass = meshWholeGrid(grid);
 stop.store(true, std::memory_order_relaxed);
 for(std::thread& writer : writers) {
  writer.join();
 }
 report("contended", pass, writes.load(std::memory_order_relaxed));
 EXPECT_GT(pass.vertices, 0u);
 // The acceptance test named in the handoff: per-cell writers are outside the
 // version protocol, so no amount of light traffic may push the capture onto the
 // chunk write lock.
 EXPECT_EQ(pass.lockedCaptures, 0u)
     << "the mesh capture fell back to Chunk::lockRenderWrite under per-cell light traffic; "
        "the look-around stall is back";
}
} // namespace net::minecraft::test
