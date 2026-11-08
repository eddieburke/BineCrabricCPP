#include "net/minecraft/client/render/distant/NetherTile.hpp"
#include <algorithm>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightType.hpp"

namespace net::minecraft::client::render::distant {
namespace {
struct FaceKey {
 std::uint8_t block = 0;
 std::uint8_t light = 0;
 bool operator==(const FaceKey&) const = default;
};
bool solid(int id) {
 if(id == 10 || id == 11) return true;
 const auto* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(id)];
 return block != nullptr && block->isOpaque();
}
void emitRectangle(std::vector<DhVertex>& vertices, const BlockPalette& palette, const FaceKey& face,
                   int normal, int axis, int uAxis, int vAxis, int slice, int u, int v,
                   int width, int height, int stride) {
 const int scale[3] = {stride, 1, stride};
 const bool positive = (normal & 1) != 0;
 int origin[3]{};
 origin[axis] = (slice + (positive ? 1 : 0)) * scale[axis];
 origin[uAxis] = u * scale[uAxis];
 origin[vAxis] = v * scale[vAxis];
 const auto& entry = palette.entries[face.block];
 const auto* rgba = normal == DhNormal::PosY ? entry.top : entry.side;
 const float shade = face.block == 10 || face.block == 11 || entry.material == DhMaterial::Illuminated
                         ? 1.0f : axis == 1 ? (positive ? 1.0f : 0.5f) : axis == 2 ? 0.8f : 0.6f;
 constexpr int corners[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
 for(int index = 0; index < 4; ++index) {
  const int corner = positive ? index : 3 - index;
  int point[3] = {origin[0], origin[1], origin[2]};
  point[uAxis] += corners[corner][0] * width * scale[uAxis];
  point[vAxis] += corners[corner][1] * height * scale[vAxis];
  DhVertex vertex;
  vertex.x = static_cast<std::uint16_t>(point[0]);
  vertex.y = static_cast<std::uint16_t>(point[1]);
  vertex.z = static_cast<std::uint16_t>(point[2]);
  vertex.meta = face.light;
  for(int channel = 0; channel < 3; ++channel)
   vertex.color[channel] = static_cast<std::uint8_t>(rgba[channel] * shade + 0.5f);
  vertex.color[3] = 255;
  vertex.material = entry.material;
  vertex.normal = static_cast<std::uint8_t>(normal);
  vertices.push_back(vertex);
 }
}
}

bool extractNetherTile(const ChunkLookup& chunks, int tileX, int tileZ, int stride,
                       NetherTileCells& out, bool allowPartial) {
 if(stride < 1 || stride > kMaxStride || (stride & (stride - 1)) != 0) return false;
 out.stride = stride;
 out.side = kTileBlocks / stride;
 out.chunkMask = 0;
 out.columns.assign(static_cast<std::size_t>((out.side + 2) * (out.side + 2)), NetherColumn{});
 constexpr int gridSide = kTileChunks + 2;
 std::array<const Chunk*, gridSide * gridSide> grid{};
 const int baseX = tileX * kTileChunks - 1;
 const int baseZ = tileZ * kTileChunks - 1;
 for(int z = 0; z < gridSide; ++z) {
  for(int x = 0; x < gridSide; ++x) {
   const Chunk* chunk = chunks(baseX + x, baseZ + z);
   grid[static_cast<std::size_t>(z * gridSide + x)] = chunk;
   if(chunk != nullptr && x > 0 && x <= kTileChunks && z > 0 && z <= kTileChunks)
    out.chunkMask |= static_cast<std::uint16_t>(1u << ((z - 1) * kTileChunks + x - 1));
  }
 }
 if(out.chunkMask == 0 || (!allowPartial && out.chunkMask != 0xFFFFu)) return false;
 std::array<std::uint8_t, 256> solidBlocks{};
 for(int id = 1; id < 256; ++id)
  solidBlocks[static_cast<std::size_t>(id)] = solid(id) ? static_cast<std::uint8_t>(id) : 0;
 for(int z = -1; z <= out.side; ++z) {
  for(int x = -1; x <= out.side; ++x) {
   const int blockX = tileX * kTileBlocks + x * stride + stride / 2;
   const int blockZ = tileZ * kTileBlocks + z * stride + stride / 2;
   const int chunkX = MathHelper::floorDiv(blockX, 16) - baseX;
   const int chunkZ = MathHelper::floorDiv(blockZ, 16) - baseZ;
   const Chunk* chunk = grid[static_cast<std::size_t>(chunkZ * gridSide + chunkX)];
   if(chunk == nullptr) continue;
   auto& column = out.at(x, z);
   column.present = true;
   for(int y = 0; y < kNetherHeight; ++y) {
    const int id = chunk->getBlockId(blockX & 15, y, blockZ & 15);
    column.blocks[static_cast<std::size_t>(y)] = solidBlocks[static_cast<std::size_t>(id)];
    column.light[static_cast<std::size_t>(y)] = static_cast<std::uint8_t>(
        chunk->getLight(LightType::Block, blockX & 15, y, blockZ & 15) << 4);
   }
  }
 }
 return true;
}

void meshNetherTile(const NetherTileCells& cells, const BlockPalette& palette, TileMesh& out) {
 out.opaque.clear();
 out.water.clear();
 out.minY = kNetherHeight;
 out.maxY = 0;
 const int dimensions[3] = {cells.side, kNetherHeight, cells.side};
 std::array<int, 256> emission{};
 for(int id = 1; id < 256; ++id) {
  const auto* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(id)];
  emission[static_cast<std::size_t>(id)] = block != nullptr ? block->emission() << 4 : 0;
 }
 std::vector<FaceKey> mask;
 for(int normal = 0; normal < 6; ++normal) {
  const int axis = normal < 2 ? 1 : normal < 4 ? 2 : 0;
  const int uAxis = (axis + 1) % 3;
  const int vAxis = (axis + 2) % 3;
  const int width = dimensions[uAxis];
  const int height = dimensions[vAxis];
  mask.resize(static_cast<std::size_t>(width * height));
  for(int slice = 0; slice < dimensions[axis]; ++slice) {
   for(int v = 0; v < height; ++v) {
    for(int u = 0; u < width; ++u) {
     FaceKey face;
     int point[3]{};
     point[axis] = slice;
     point[uAxis] = u;
     point[vAxis] = v;
     const auto& column = cells.at(point[0], point[2]);
     const int id = column.blocks[static_cast<std::size_t>(point[1])];
     if(column.present && id != 0) {
      point[axis] += (normal & 1) != 0 ? 1 : -1;
      const auto& neighbor = cells.at(point[0], point[2]);
      const bool inHeight = point[1] >= 0 && point[1] < kNetherHeight;
      if(!inHeight || !neighbor.present || neighbor.blocks[static_cast<std::size_t>(point[1])] == 0) {
       face.block = static_cast<std::uint8_t>(id);
       const int light = inHeight && neighbor.present ? neighbor.light[static_cast<std::size_t>(point[1])] : 0;
       face.light = static_cast<std::uint8_t>(std::max(light, emission[static_cast<std::size_t>(id)]));
      }
     }
     mask[static_cast<std::size_t>(v * width + u)] = face;
    }
   }
   for(int v = 0; v < height; ++v) {
    for(int u = 0; u < width;) {
     const FaceKey face = mask[static_cast<std::size_t>(v * width + u)];
     if(face.block == 0) { ++u; continue; }
     int runWidth = 1;
     while(u + runWidth < width && mask[static_cast<std::size_t>(v * width + u + runWidth)] == face) ++runWidth;
     int runHeight = 1;
     bool same = true;
     while(v + runHeight < height && same) {
      for(int offset = 0; offset < runWidth; ++offset) {
       if(mask[static_cast<std::size_t>((v + runHeight) * width + u + offset)] != face) { same = false; break; }
      }
      if(same) ++runHeight;
     }
     emitRectangle(out.opaque, palette, face, normal, axis, uAxis, vAxis, slice, u, v, runWidth, runHeight, cells.stride);
     for(int row = 0; row < runHeight; ++row)
      std::fill_n(mask.begin() + (v + row) * width + u, runWidth, FaceKey{});
     u += runWidth;
    }
   }
  }
 }
 for(const auto& vertex : out.opaque) {
  out.minY = std::min<int>(out.minY, vertex.y);
  out.maxY = std::max<int>(out.maxY, vertex.y);
 }
 if(out.opaque.empty()) out.minY = 0;
}
}
