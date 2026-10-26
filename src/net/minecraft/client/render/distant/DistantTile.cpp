#include "net/minecraft/client/render/distant/DistantTile.hpp"
#include <algorithm>
#include <cstring>
#include "net/minecraft/client/color/world/FoliageColors.hpp"
#include "net/minecraft/client/color/world/GrassColors.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/QuadIndexBuffer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightType.hpp"
// see LOD_DESIGN.md §3, §5, §6
namespace net::minecraft::client::render::distant {
namespace {
constexpr unsigned kArrayBuffer = 0x8892;
constexpr unsigned kDynamicDraw = 0x88E8;
constexpr unsigned kDynamicStorageBit = 0x0100;
constexpr std::size_t kIndexCoverageVertices = 262144;
constexpr int kBorderChunks = kTileChunks + 2;
constexpr std::uint8_t kWaterStill = 9;
constexpr std::uint8_t kWaterFlowing = 8;
constexpr int kLog = 17;
constexpr int kLeaves = 18;

constexpr std::array<bool, 256> makeSkipTable() {
 std::array<bool, 256> skip{};
 constexpr int ids[] = {0,  6,  20, 27, 28, 30, 31, 32, 36, 37, 38, 39, 40, 50, 51, 55, 59, 63,
                        64, 65, 66, 68, 69, 70, 71, 72, 75, 76, 77, 83, 90, 92, 93, 94, 96};
 for(const int id : ids) {
  skip[static_cast<std::size_t>(id)] = true;
 }
 return skip;
}
constexpr std::array<bool, 256> kSkip = makeSkipTable();

struct ColumnSample {
 std::array<ColumnRun, kMaxRuns> runs{};
 int runCount = 0;
 int waterTop = 0;
 std::uint8_t waterLight = 0;
};

[[nodiscard]] std::uint8_t packedLight(const Chunk& chunk, int lx, int y, int lz) {
 if(y >= Chunk::height) {
  return 0x0F;
 }
 if(y < 0) {
  return 0;
 }
 return static_cast<std::uint8_t>((chunk.getLight(LightType::Block, lx, y, lz) << 4) |
                                  chunk.getLight(LightType::Sky, lx, y, lz));
}

[[nodiscard]] int topScanY(const Chunk& chunk) {
 int top = -1;
 for(int section = Chunk::height / 16 - 1; section >= 0; --section) {
  if(chunk.sectionHasBlocks(section)) {
   top = section * 16 + 15;
   break;
  }
 }
 for(int z = 0; z < 16; ++z) {
  for(int x = 0; x < 16; ++x) {
   top = std::max(top, std::min(chunk.getHeight(x, z), Chunk::height - 1));
  }
 }
 return top < 0 ? Chunk::height - 1 : top;
}

void sampleColumn(const Chunk& chunk, int lx, int lz, int startY, ColumnSample& sample) {
 sample = ColumnSample{};
 const int height = chunk.getHeight(lx, lz);
 bool inRun = false;
 ColumnRun run;
 const auto close = [&](int y) {
  run.bottom = static_cast<std::uint8_t>(y + 1);
  run.bottomLight = packedLight(chunk, lx, y, lz);
  if(run.block != 0) {
   sample.runs[static_cast<std::size_t>(sample.runCount++)] = run;
  }
  inRun = false;
 };
 for(int y = startY; y >= 0 && sample.runCount < kMaxRuns; --y) {
  const int id = chunk.getBlockId(lx, y, lz);
  const bool water = id == kWaterStill || id == kWaterFlowing;
  const bool open = water || (kSkip[static_cast<std::size_t>(id)] &&
                              (y >= height || chunk.getLight(LightType::Sky, lx, y, lz) > 0));
  if(open) {
   if(inRun) {
    close(y);
   }
   if(water && sample.waterTop == 0 && sample.runCount == 0) {
    sample.waterTop = y + 1;
    sample.waterLight = packedLight(chunk, lx, y + 1, lz);
   }
   continue;
  }
  const bool solid = !kSkip[static_cast<std::size_t>(id)];
  if(!inRun) {
   inRun = true;
   run = ColumnRun{};
   run.top = static_cast<std::uint8_t>(y + 1);
   run.topLight = packedLight(chunk, lx, y + 1, lz);
  }
  if(solid && run.block == 0) {
   run.block = static_cast<std::uint8_t>(id);
   run.meta = static_cast<std::uint8_t>(chunk.getBlockMeta(lx, y, lz));
  }
 }
 if(inRun && sample.runCount < kMaxRuns) {
  run.bottom = 0;
  run.bottomLight = 0;
  if(run.block != 0) {
   sample.runs[static_cast<std::size_t>(sample.runCount++)] = run;
  }
 }
}

[[nodiscard]] bool canopyTop(const ColumnSample& sample) {
 return sample.runCount > 0 && (sample.runs[0].block == kLeaves || sample.runs[0].block == kLog);
}

void unpackColor(int rgb, std::uint8_t (&out)[3]) {
 out[0] = static_cast<std::uint8_t>((rgb >> 16) & 0xFF);
 out[1] = static_cast<std::uint8_t>((rgb >> 8) & 0xFF);
 out[2] = static_cast<std::uint8_t>(rgb & 0xFF);
}

[[nodiscard]] std::uint8_t shadeChannel(std::uint8_t value, float shade, std::uint8_t tint) noexcept {
 return static_cast<std::uint8_t>(static_cast<float>(value) * shade * (static_cast<float>(tint) / 255.0f) + 0.5f);
}

struct Face {
 const std::uint8_t* rgba = nullptr;
 const std::uint8_t* tint = nullptr;
 std::uint8_t material = 0;
 std::uint8_t light = 0;
};

constexpr std::uint8_t kNoTint[3] = {255, 255, 255};

void pushQuad(std::vector<DhVertex>& out,
              const std::uint16_t (&xs)[4],
              const std::uint16_t (&ys)[4],
              const std::uint16_t (&zs)[4],
              const Face& face,
              float shade,
              std::uint8_t normal) {
 DhVertex vertex;
 vertex.meta = face.light;
 vertex.color[0] = shadeChannel(face.rgba[0], shade, face.tint[0]);
 vertex.color[1] = shadeChannel(face.rgba[1], shade, face.tint[1]);
 vertex.color[2] = shadeChannel(face.rgba[2], shade, face.tint[2]);
 vertex.color[3] = face.rgba[3];
 vertex.material = face.material;
 vertex.normal = normal;
 for(int i = 0; i < 4; ++i) {
  vertex.x = xs[i];
  vertex.y = ys[i];
  vertex.z = zs[i];
  out.push_back(vertex);
 }
}

[[nodiscard]] std::uint16_t u16(int value) noexcept {
 return static_cast<std::uint16_t>(value);
}

void pushTop(std::vector<DhVertex>& out, int x0, int x1, int z0, int z1, int y, const Face& face) {
 const std::uint16_t xs[4] = {u16(x0), u16(x0), u16(x1), u16(x1)};
 const std::uint16_t zs[4] = {u16(z0), u16(z1), u16(z1), u16(z0)};
 const std::uint16_t ys[4] = {u16(y), u16(y), u16(y), u16(y)};
 pushQuad(out, xs, ys, zs, face, 1.0f, DhNormal::PosY);
}

void pushBottom(std::vector<DhVertex>& out, int x0, int x1, int z0, int z1, int y, const Face& face) {
 const std::uint16_t xs[4] = {u16(x0), u16(x1), u16(x1), u16(x0)};
 const std::uint16_t zs[4] = {u16(z0), u16(z0), u16(z1), u16(z1)};
 const std::uint16_t ys[4] = {u16(y), u16(y), u16(y), u16(y)};
 pushQuad(out, xs, ys, zs, face, 0.5f, DhNormal::NegY);
}

struct SideDir {
 int dx;
 int dz;
 std::uint8_t normal;
 float shade;
};
constexpr SideDir kSideDirs[4] = {{0, -1, DhNormal::NegZ, 0.8f},
                                  {0, 1, DhNormal::PosZ, 0.8f},
                                  {-1, 0, DhNormal::NegX, 0.6f},
                                  {1, 0, DhNormal::PosX, 0.6f}};

void pushSide(std::vector<DhVertex>& out, const SideDir& dir, int x0, int x1, int z0, int z1, int yLow, int yHigh,
              const Face& face) {
 const std::uint16_t ys[4] = {u16(yLow), u16(yHigh), u16(yHigh), u16(yLow)};
 std::uint16_t xs[4];
 std::uint16_t zs[4];
 if(dir.normal == DhNormal::NegZ) {
  xs[0] = xs[1] = u16(x0);
  xs[2] = xs[3] = u16(x1);
  zs[0] = zs[1] = zs[2] = zs[3] = u16(z0);
 } else if(dir.normal == DhNormal::PosZ) {
  xs[0] = xs[1] = u16(x1);
  xs[2] = xs[3] = u16(x0);
  zs[0] = zs[1] = zs[2] = zs[3] = u16(z1);
 } else if(dir.normal == DhNormal::NegX) {
  zs[0] = zs[1] = u16(z1);
  zs[2] = zs[3] = u16(z0);
  xs[0] = xs[1] = xs[2] = xs[3] = u16(x0);
 } else {
  zs[0] = zs[1] = u16(z0);
  zs[2] = zs[3] = u16(z1);
  xs[0] = xs[1] = xs[2] = xs[3] = u16(x1);
 }
 pushQuad(out, xs, ys, zs, face, dir.shade, dir.normal);
}

[[nodiscard]] const std::uint8_t* tintFor(const PaletteEntry& entry, const ColumnCell& cell, const ColumnRun& run,
                                          bool topFace, std::uint8_t (&scratch)[3]) {
 if(entry.tint == PaletteTint::GrassTint && topFace) {
  return cell.grass;
 }
 if(entry.tint == PaletteTint::FoliageTint) {
  if((run.meta & 1) != 0) {
   unpackColor(client::color::world::FoliageColors::getSpruceColor(), scratch);
   return scratch;
  }
  if((run.meta & 2) != 0) {
   unpackColor(client::color::world::FoliageColors::getBirchColor(), scratch);
   return scratch;
  }
  return cell.foliage;
 }
 return kNoTint;
}

struct PaletteBuilder {
 BlockPalette& palette;
 void set(int id, std::uint32_t topRgb, std::uint32_t sideRgb, DhMaterial material, bool translucent = false,
          std::uint8_t alpha = 255, std::uint8_t tint = PaletteTint::NoTint) const {
  PaletteEntry& entry = palette.entries[static_cast<std::size_t>(id)];
  entry.top[0] = static_cast<std::uint8_t>(topRgb >> 16);
  entry.top[1] = static_cast<std::uint8_t>(topRgb >> 8);
  entry.top[2] = static_cast<std::uint8_t>(topRgb);
  entry.top[3] = alpha;
  entry.side[0] = static_cast<std::uint8_t>(sideRgb >> 16);
  entry.side[1] = static_cast<std::uint8_t>(sideRgb >> 8);
  entry.side[2] = static_cast<std::uint8_t>(sideRgb);
  entry.side[3] = alpha;
  entry.material = material;
  entry.translucent = translucent;
  entry.tint = tint;
 }
 void same(int id, std::uint32_t rgb, DhMaterial material) const {
  set(id, rgb, rgb, material);
 }
};
} // namespace

BlockPalette defaultPalette() {
 BlockPalette palette;
 const PaletteBuilder b{palette};
 for(int id = 1; id < 256; ++id) {
  b.same(id, 0x808080, DhMaterial::Stone);
 }
 b.set(0, 0, 0, DhMaterial::Air, false, 0);
 b.same(1, 0x7d7d7d, DhMaterial::Stone);
 b.set(2, 0x969696, 0x7f6a45, DhMaterial::Grass, false, 255, PaletteTint::GrassTint);
 b.same(3, 0x866043, DhMaterial::Dirt);
 b.same(4, 0x7a7a7a, DhMaterial::Stone);
 b.same(5, 0x9c7f4e, DhMaterial::Wood);
 b.same(7, 0x535353, DhMaterial::Stone);
 b.set(8, 0x3f76e4, 0x3f76e4, DhMaterial::Water, true, 160);
 b.set(9, 0x3f76e4, 0x3f76e4, DhMaterial::Water, true, 160);
 b.same(10, 0xd96514, DhMaterial::Lava);
 b.same(11, 0xd96514, DhMaterial::Lava);
 b.same(12, 0xdbd3a0, DhMaterial::Sand);
 b.same(13, 0x857f7e, DhMaterial::Dirt);
 b.same(14, 0x8f8b7c, DhMaterial::Stone);
 b.same(15, 0x88837e, DhMaterial::Stone);
 b.same(16, 0x737373, DhMaterial::Stone);
 b.set(17, 0x6b5839, 0x665132, DhMaterial::Wood);
 b.set(18, 0x8c8c8c, 0x8c8c8c, DhMaterial::Leaves, false, 255, PaletteTint::FoliageTint);
 b.same(19, 0xc3c34a, DhMaterial::Dirt);
 b.same(21, 0x6e7a8f, DhMaterial::Stone);
 b.same(22, 0x1f3fa3, DhMaterial::Stone);
 b.same(23, 0x6e6e6e, DhMaterial::Stone);
 b.same(24, 0xd8cf9b, DhMaterial::Sand);
 b.same(25, 0x6e4a33, DhMaterial::Wood);
 b.same(26, 0xa02020, DhMaterial::Wood);
 b.same(29, 0x8a7b52, DhMaterial::Stone);
 b.same(33, 0x8a7b52, DhMaterial::Stone);
 b.same(34, 0x8a7b52, DhMaterial::Stone);
 b.same(35, 0xdcdcdc, DhMaterial::Unknown);
 b.same(41, 0xf7e84a, DhMaterial::Metal);
 b.same(42, 0xdcdcdc, DhMaterial::Metal);
 b.same(43, 0xa8a8a8, DhMaterial::Stone);
 b.same(44, 0xa8a8a8, DhMaterial::Stone);
 b.same(45, 0x96564a, DhMaterial::Stone);
 b.same(46, 0xc04030, DhMaterial::Unknown);
 b.same(47, 0x6c5533, DhMaterial::Wood);
 b.same(48, 0x5f7a5f, DhMaterial::Stone);
 b.same(49, 0x14121e, DhMaterial::Stone);
 b.same(52, 0x1b2a3a, DhMaterial::Stone);
 b.same(53, 0x9c7f4e, DhMaterial::Wood);
 b.same(54, 0xa07a35, DhMaterial::Wood);
 b.same(56, 0x83898a, DhMaterial::Stone);
 b.same(57, 0x62dbd5, DhMaterial::Metal);
 b.same(58, 0x8a6a3a, DhMaterial::Wood);
 b.same(60, 0x5a3f26, DhMaterial::Dirt);
 b.same(61, 0x6e6e6e, DhMaterial::Stone);
 b.same(62, 0x6e6e6e, DhMaterial::Stone);
 b.same(67, 0x7a7a7a, DhMaterial::Stone);
 b.same(73, 0x8a6e6e, DhMaterial::Stone);
 b.same(74, 0x8a6e6e, DhMaterial::Stone);
 b.same(78, 0xeefbfb, DhMaterial::Snow);
 b.same(79, 0x7dadff, DhMaterial::Snow);
 b.same(80, 0xf0fbfb, DhMaterial::Snow);
 b.set(81, 0x0d6b1d, 0x0e6d1e, DhMaterial::Grass);
 b.same(82, 0xa0a6b4, DhMaterial::Dirt);
 b.same(84, 0x6a4a33, DhMaterial::Wood);
 b.same(85, 0x9c7f4e, DhMaterial::Wood);
 b.set(86, 0xc58a1a, 0xc07612, DhMaterial::Wood);
 b.same(87, 0x6e3535, DhMaterial::NetherStone);
 b.same(88, 0x544033, DhMaterial::NetherStone);
 b.same(89, 0xab8654, DhMaterial::Illuminated);
 b.same(91, 0xc68f1a, DhMaterial::Illuminated);
 b.same(95, 0xa07a35, DhMaterial::Wood);
 return palette;
}

bool extractTile(const ChunkLookup& chunks, int tileX, int tileZ, int stride, TileCells& out,
                 const BiomeSource* biomes) {
 out.tileX = tileX;
 out.tileZ = tileZ;
 out.stride = stride;
 out.side = kTileBlocks / stride;
 const int side = out.side;
 out.cells.assign(static_cast<std::size_t>((side + 2) * (side + 2)), ColumnCell{});
 const int baseChunkX = tileX * kTileChunks - 1;
 const int baseChunkZ = tileZ * kTileChunks - 1;
 const Chunk* grid[kBorderChunks * kBorderChunks];
 int scanTop[kBorderChunks * kBorderChunks];
 for(int cz = 0; cz < kBorderChunks; ++cz) {
  for(int cx = 0; cx < kBorderChunks; ++cx) {
   const Chunk* chunk = chunks(baseChunkX + cx, baseChunkZ + cz);
   grid[cz * kBorderChunks + cx] = chunk;
   scanTop[cz * kBorderChunks + cx] = chunk != nullptr ? topScanY(*chunk) : -1;
  }
 }
 for(int cz = 1; cz <= kTileChunks; ++cz) {
  for(int cx = 1; cx <= kTileChunks; ++cx) {
   if(grid[cz * kBorderChunks + cx] == nullptr) {
    return false;
   }
  }
 }
 const int originX = tileX * kTileBlocks;
 const int originZ = tileZ * kTileBlocks;
 const int columns = stride * stride;
 ColumnSample sample;
 ColumnSample bestCanopy;
 ColumnSample bestGround;
 for(int cellZ = -1; cellZ <= side; ++cellZ) {
  for(int cellX = -1; cellX <= side; ++cellX) {
   const int blockX = originX + cellX * stride;
   const int blockZ = originZ + cellZ * stride;
   const int gridIndex = ((blockZ >> 4) - baseChunkZ) * kBorderChunks + ((blockX >> 4) - baseChunkX);
   const Chunk* chunk = grid[gridIndex];
   if(chunk == nullptr) {
    continue;
   }
   const int startY = scanTop[gridIndex];
   const int localX0 = blockX & 15;
   const int localZ0 = blockZ & 15;
   int canopyCount = 0;
   int waterCount = 0;
   int waterTop = 0;
   std::uint8_t waterLight = 0;
   bool haveCanopy = false;
   bool haveGround = false;
   for(int dz = 0; dz < stride; ++dz) {
    for(int dx = 0; dx < stride; ++dx) {
     sampleColumn(*chunk, localX0 + dx, localZ0 + dz, startY, sample);
     if(sample.waterTop > 0) {
      ++waterCount;
      if(sample.waterTop > waterTop) {
       waterTop = sample.waterTop;
       waterLight = sample.waterLight;
      }
     }
     if(sample.runCount == 0) {
      continue;
     }
     if(canopyTop(sample)) {
      ++canopyCount;
      if(!haveCanopy || sample.runs[0].top > bestCanopy.runs[0].top) {
       bestCanopy = sample;
       haveCanopy = true;
      }
     } else if(!haveGround || sample.runs[0].top > bestGround.runs[0].top) {
      bestGround = sample;
      haveGround = true;
     }
    }
   }
   ColumnCell& cell = out.at(cellX, cellZ);
   cell.present = true;
   const ColumnSample* chosen = nullptr;
   if(haveCanopy && (canopyCount * 2 >= columns || !haveGround)) {
    chosen = &bestCanopy;
   } else if(haveGround) {
    chosen = &bestGround;
   }
   if(chosen != nullptr) {
    cell.runCount = static_cast<std::uint8_t>(chosen->runCount);
    cell.runs = chosen->runs;
   }
   if(waterCount * 2 >= columns && (cell.runCount == 0 || waterTop > cell.runs[0].top)) {
    cell.waterTop = static_cast<std::uint8_t>(waterTop);
    cell.waterLight = waterLight;
   }
   if(biomes != nullptr) {
    const auto climate = biomes->sampleClimate(blockX + stride / 2, blockZ + stride / 2);
    unpackColor(client::color::world::GrassColors::getColor(climate.temperature, climate.downfall), cell.grass);
    unpackColor(client::color::world::FoliageColors::getColor(climate.temperature, climate.downfall), cell.foliage);
   }
  }
 }
 return true;
}

void meshTile(const TileCells& cells, const BlockPalette& palette, TileMesh& out) {
 out.opaque.clear();
 out.water.clear();
 const int side = cells.side;
 const int stride = cells.stride;
 const PaletteEntry& water = palette.entries[kWaterStill];
 std::uint8_t scratch[3];
 std::uint8_t neighborScratch[3];
 for(int cz = 0; cz < side; ++cz) {
  int cx = 0;
  while(cx < side) {
   const ColumnCell& c = cells.at(cx, cz);
   if(!c.present || c.runCount == 0) {
    ++cx;
    continue;
   }
   const ColumnRun& r = c.runs[0];
   const PaletteEntry& entry = palette.entries[r.block];
   const std::uint8_t* tint = tintFor(entry, c, r, true, scratch);
   int end = cx + 1;
   while(end < side) {
    const ColumnCell& n = cells.at(end, cz);
    if(!n.present || n.runCount == 0) {
     break;
    }
    const ColumnRun& nr = n.runs[0];
    if(nr.top != r.top || nr.block != r.block || nr.meta != r.meta || nr.topLight != r.topLight ||
       std::memcmp(tintFor(entry, n, nr, true, neighborScratch), tint, 3) != 0) {
     break;
    }
    ++end;
   }
   pushTop(out.opaque, cx * stride, end * stride, cz * stride, (cz + 1) * stride, r.top,
           Face{entry.top, tint, entry.material, r.topLight});
   cx = end;
  }
 }
 for(int cz = 0; cz < side; ++cz) {
  for(int cx = 0; cx < side; ++cx) {
   const ColumnCell& c = cells.at(cx, cz);
   if(!c.present) {
    continue;
   }
   const int x0 = cx * stride;
   const int z0 = cz * stride;
   for(int index = 0; index < c.runCount; ++index) {
    const ColumnRun& r = c.runs[static_cast<std::size_t>(index)];
    const PaletteEntry& entry = palette.entries[r.block];
    if(index > 0) {
     pushTop(out.opaque, x0, x0 + stride, z0, z0 + stride, r.top,
             Face{entry.top, tintFor(entry, c, r, true, scratch), entry.material, r.topLight});
    }
    const std::uint8_t* sideTint = tintFor(entry, c, r, false, scratch);
    if(r.bottom > 0) {
     pushBottom(out.opaque, x0, x0 + stride, z0, z0 + stride, r.bottom,
                Face{entry.side, sideTint, entry.material, r.bottomLight});
    }
    for(const SideDir& dir : kSideDirs) {
     const ColumnCell& n = cells.at(cx + dir.dx, cz + dir.dz);
     if(!n.present) {
      pushSide(out.opaque, dir, x0, x0 + stride, z0, z0 + stride, r.bottom, r.top,
               Face{entry.side, sideTint, entry.material, r.topLight});
      continue;
     }
     int cursor = r.top;
     for(int k = 0; k < n.runCount && cursor > r.bottom; ++k) {
      const ColumnRun& nr = n.runs[static_cast<std::size_t>(k)];
      if(nr.top <= r.bottom) {
       break;
      }
      if(nr.bottom >= cursor) {
       continue;
      }
      if(nr.top < cursor) {
       pushSide(out.opaque, dir, x0, x0 + stride, z0, z0 + stride, std::max<int>(nr.top, r.bottom), cursor,
                Face{entry.side, sideTint, entry.material, nr.topLight});
      }
      cursor = std::min<int>(cursor, nr.bottom);
     }
     if(cursor > r.bottom) {
      pushSide(out.opaque, dir, x0, x0 + stride, z0, z0 + stride, r.bottom, cursor,
               Face{entry.side, sideTint, entry.material, r.topLight});
     }
    }
   }
  }
 }
 for(int cz = 0; cz < side; ++cz) {
  int cx = 0;
  while(cx < side) {
   const ColumnCell& c = cells.at(cx, cz);
   if(!c.present || c.waterTop == 0) {
    ++cx;
    continue;
   }
   int end = cx + 1;
   while(end < side) {
    const ColumnCell& n = cells.at(end, cz);
    if(!n.present || n.waterTop != c.waterTop || n.waterLight != c.waterLight) {
     break;
    }
    ++end;
   }
   pushTop(out.water, cx * stride, end * stride, cz * stride, (cz + 1) * stride, c.waterTop,
           Face{water.top, kNoTint, DhMaterial::Water, c.waterLight});
   cx = end;
  }
 }
 for(int cz = 0; cz < side; ++cz) {
  for(int cx = 0; cx < side; ++cx) {
   const ColumnCell& c = cells.at(cx, cz);
   if(!c.present || c.waterTop == 0) {
    continue;
   }
   const int floor = c.runCount > 0 ? c.runs[0].top : 0;
   for(const SideDir& dir : kSideDirs) {
    if(cells.at(cx + dir.dx, cz + dir.dz).present) {
     continue;
    }
    pushSide(out.water, dir, cx * stride, (cx + 1) * stride, cz * stride, (cz + 1) * stride, floor, c.waterTop,
             Face{water.side, kNoTint, DhMaterial::Water, c.waterLight});
   }
  }
 }
 bool any = false;
 int minY = 0;
 int maxY = 0;
 for(const std::vector<DhVertex>* list : {&out.opaque, &out.water}) {
  for(const DhVertex& vertex : *list) {
   if(!any) {
    minY = maxY = vertex.y;
    any = true;
   } else {
    minY = std::min<int>(minY, vertex.y);
    maxY = std::max<int>(maxY, vertex.y);
   }
  }
 }
 out.minY = minY;
 out.maxY = maxY;
}

bool DistantSlab::create(std::size_t bytes) {
 destroy();
 constexpr std::size_t kPageBytes = kPageVertices * sizeof(DhVertex);
 const std::size_t pages = (bytes + kPageBytes - 1) / kPageBytes;
 if(pages == 0 || gl::GLCore::genBuffers == nullptr || gl::GLCore::bindBuffer == nullptr ||
    gl::GLCore::genVertexArrays == nullptr || (gl::GLCore::bufferStorage == nullptr && gl::GLCore::bufferData == nullptr) ||
    !quad_index::ensure(std::min(pages * kPageVertices, kIndexCoverageVertices))) {
  return false;
 }
 unsigned handle = 0;
 gl::GLCore::genBuffers(1, &handle);
 if(handle == 0) {
  return false;
 }
 buffer_ = gl::GlBuffer(handle);
 gl::GLCore::bindBuffer(kArrayBuffer, handle);
 const auto size = static_cast<intptr_t>(pages * kPageBytes);
 if(gl::GLCore::bufferStorage != nullptr) {
  gl::GLCore::bufferStorage(kArrayBuffer, size, nullptr, kDynamicStorageBit);
 } else {
  gl::GLCore::bufferData(kArrayBuffer, size, nullptr, kDynamicDraw);
 }
 gl::GLCore::bindBuffer(kArrayBuffer, 0);
 unsigned vao = 0;
 gl::GLCore::genVertexArrays(1, &vao);
 vao_ = gl::GlVao(vao);
 using vertex_abi::Availability;
 using vertex_abi::Format;
 static constexpr Format kFormats[3] = {{0, 4, 0x1403, false, true, 0, Availability::Always},
                                         {1, 4, 0x1401, true, false, 8, Availability::Always},
                                         {2, 4, 0x1401, false, true, 12, Availability::Always}};
 if(vao == 0 || !core::configureIndexedVao(vao, handle, quad_index::handle(), static_cast<int>(sizeof(DhVertex)),
                                           kFormats)) {
  destroy();
  return false;
 }
 pageCount_ = static_cast<std::uint32_t>(pages);
 usedPages_ = 0;
 freeRuns_.assign(1, {0u, pageCount_});
 return true;
}

void DistantSlab::destroy() {
 vao_.reset();
 buffer_.reset();
 freeRuns_.clear();
 pageCount_ = 0;
 usedPages_ = 0;
}

std::optional<DistantSlab::Allocation> DistantSlab::allocate(std::size_t vertexCount) {
 const auto pages = static_cast<std::uint32_t>((vertexCount + kPageVertices - 1) / kPageVertices);
 Allocation allocation;
 allocation.vertexCount = static_cast<std::uint32_t>(vertexCount);
 allocation.pageCount = pages;
 if(pages == 0) {
  return allocation;
 }
 for(auto it = freeRuns_.begin(); it != freeRuns_.end(); ++it) {
  if(it->second < pages) {
   continue;
  }
  allocation.firstPage = it->first;
  if(it->second == pages) {
   freeRuns_.erase(it);
  } else {
   it->first += pages;
   it->second -= pages;
  }
  usedPages_ += pages;
  return allocation;
 }
 return std::nullopt;
}

void DistantSlab::release(const Allocation& allocation) {
 if(allocation.pageCount == 0) {
  return;
 }
 usedPages_ -= allocation.pageCount;
 auto it = std::lower_bound(freeRuns_.begin(), freeRuns_.end(), allocation.firstPage,
                            [](const std::pair<std::uint32_t, std::uint32_t>& run, std::uint32_t page) {
                             return run.first < page;
                            });
 it = freeRuns_.insert(it, {allocation.firstPage, allocation.pageCount});
 auto next = it + 1;
 if(next != freeRuns_.end() && it->first + it->second == next->first) {
  it->second += next->second;
  freeRuns_.erase(next);
 }
 if(it != freeRuns_.begin()) {
  auto prev = it - 1;
  if(prev->first + prev->second == it->first) {
   prev->second += it->second;
   freeRuns_.erase(it);
  }
 }
}

void DistantSlab::upload(const Allocation& allocation, std::span<const DhVertex> vertices) {
 if(!buffer_ || vertices.empty() || gl::GLCore::bufferSubData == nullptr) {
  return;
 }
 const std::size_t count = std::min<std::size_t>(vertices.size(), static_cast<std::size_t>(allocation.pageCount) * kPageVertices);
 gl::GLCore::bindBuffer(kArrayBuffer, buffer_.handle());
 gl::GLCore::bufferSubData(kArrayBuffer,
                           static_cast<intptr_t>(static_cast<std::size_t>(allocation.baseVertex()) * sizeof(DhVertex)),
                           static_cast<intptr_t>(count * sizeof(DhVertex)),
                           vertices.data());
 gl::GLCore::bindBuffer(kArrayBuffer, 0);
}

unsigned int DistantSlab::vao() const noexcept {
 return vao_.handle();
}

std::size_t DistantSlab::capacityBytes() const noexcept {
 return static_cast<std::size_t>(pageCount_) * kPageVertices * sizeof(DhVertex);
}

std::size_t DistantSlab::usedBytes() const noexcept {
 return static_cast<std::size_t>(usedPages_) * kPageVertices * sizeof(DhVertex);
}
} // namespace net::minecraft::client::render::distant
