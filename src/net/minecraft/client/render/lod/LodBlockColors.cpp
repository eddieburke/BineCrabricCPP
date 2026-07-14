#include "net/minecraft/client/render/lod/LodBlockColors.hpp"
#include <algorithm>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/MapColor.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::render::lod {
namespace {
bool g_built = false;
std::array<std::uint32_t, 256> g_table{};
std::array<std::uint32_t, 256> g_sideTable{};
[[nodiscard]] std::uint32_t packRgba(int r, int g, int b) {
  r = std::clamp(r, 0, 255);
  g = std::clamp(g, 0, 255);
  b = std::clamp(b, 0, 255);
  return static_cast<std::uint32_t>(r) | (static_cast<std::uint32_t>(g) << 8) |
         (static_cast<std::uint32_t>(b) << 16) | 0xFF000000U;
}
[[nodiscard]] std::uint32_t tint(std::uint32_t rgba, int tr, int tg, int tb) {
  const int r = static_cast<int>(rgba & 0xFFU) * tr / 255;
  const int g = static_cast<int>((rgba >> 8) & 0xFFU) * tg / 255;
  const int b = static_cast<int>((rgba >> 16) & 0xFFU) * tb / 255;
  return packRgba(r, g, b);
}
[[nodiscard]] std::uint32_t averageTile(const net::minecraft::client::texture::RasterImage& atlas, int tileIndex) {
  if(atlas.width < 256 || atlas.height < 256 || tileIndex < 0 || tileIndex > 255 ||
     atlas.argb.size() < static_cast<std::size_t>(atlas.width) * static_cast<std::size_t>(atlas.height)) {
    return 0;
  }
  const int tileW = atlas.width / 16;
  const int tileH = atlas.height / 16;
  const int baseX = (tileIndex % 16) * tileW;
  const int baseY = (tileIndex / 16) * tileH;
  long long sumR = 0;
  long long sumG = 0;
  long long sumB = 0;
  long long count = 0;
  for(int py = 0; py < tileH; ++py) {
    const std::size_t row = static_cast<std::size_t>(baseY + py) * static_cast<std::size_t>(atlas.width);
    for(int px = 0; px < tileW; ++px) {
      const std::uint32_t argb = atlas.argb[row + static_cast<std::size_t>(baseX + px)];
      if(((argb >> 24) & 0xFFU) < 0x80U) {
        continue;
      }
      sumR += (argb >> 16) & 0xFFU;
      sumG += (argb >> 8) & 0xFFU;
      sumB += argb & 0xFFU;
      ++count;
    }
  }
  if(count == 0) {
    return 0;
  }
  return packRgba(static_cast<int>(sumR / count), static_cast<int>(sumG / count), static_cast<int>(sumB / count));
}
[[nodiscard]] std::uint32_t mapColorFallback(const Block* block) {
  if(block == nullptr) {
    return 0;
  }
  const int color = block->material.mapColor.color;
  if(color == 0) {
    return 0;
  }
  return packRgba((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}
void build() {
  g_table.fill(0);
  g_sideTable.fill(0);
  net::minecraft::client::texture::RasterImage atlas;
  if(client::Minecraft::INSTANCE != nullptr) {
    atlas = client::Minecraft::INSTANCE->textureManager.loadRasterForResource("/terrain.png");
  }
  for(int id = 1; id < Block::BLOCK_COUNT; ++id) {
    Block* block = Block::BLOCKS[static_cast<std::size_t>(id)];
    if(block == nullptr) {
      continue;
    }
    const std::uint32_t fallback = mapColorFallback(block);
    std::uint32_t top = averageTile(atlas, block->getTexture(Block::FACE_TOP));
    if(top == 0) {
      top = fallback;
    }
    std::uint32_t side = averageTile(atlas, block->getTexture(Block::FACE_NORTH));
    if(side == 0) {
      side = top;
    }
    if(top == 0) {
      continue;
    }
    g_table[static_cast<std::size_t>(id)] = top;
    g_sideTable[static_cast<std::size_t>(id)] = side;
  }
  if(g_table[2] != 0) {
    g_table[2] = tint(g_table[2], 0x7C, 0xBD, 0x6B);
  }
  if(g_table[18] != 0) {
    g_table[18] = tint(g_table[18], 0x62, 0xA8, 0x44);
  }
  if(g_sideTable[18] != 0) {
    g_sideTable[18] = tint(g_sideTable[18], 0x62, 0xA8, 0x44);
  }
  g_table[8] = packRgba(0x2C, 0x4D, 0xC8);
  g_table[9] = g_table[8];
  g_sideTable[8] = g_table[8];
  g_sideTable[9] = g_table[8];
  if(g_table[10] != 0) {
    g_table[11] = g_table[10];
  } else {
    g_table[10] = packRgba(0xD4, 0x5A, 0x12);
    g_table[11] = g_table[10];
  }
  if(g_sideTable[10] == 0) {
    g_sideTable[10] = g_table[10];
  }
  g_sideTable[11] = g_sideTable[10];
  g_built = true;
}
} // namespace
const std::array<std::uint32_t, 256>& LodBlockColors::table() {
  if(!g_built) {
    build();
  }
  return g_table;
}
const std::array<std::uint32_t, 256>& LodBlockColors::sideTable() {
  if(!g_built) {
    build();
  }
  return g_sideTable;
}
void LodBlockColors::invalidate() {
  g_built = false;
}
} // namespace net::minecraft::client::render::lod
