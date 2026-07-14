#pragma once
#include <array>
#include <cstdint>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft::client::render::lod {
struct LodColumn {
  std::uint8_t topY = 0;
  std::uint8_t topBlock = 0;
  std::uint8_t waterY = 0;
  std::uint8_t flags = 0;
  [[nodiscard]] bool hasSurface() const noexcept {
    return topBlock != 0 || waterY != 0;
  }
  [[nodiscard]] int effectiveTop() const noexcept {
    return waterY > topY ? waterY : topY;
  }
};
struct LodChunk {
  std::array<LodColumn, 256> columns{};
  bool populated = false;
  [[nodiscard]] const LodColumn& at(int localX, int localZ) const noexcept {
    return columns[static_cast<std::size_t>((localZ << 4) | localX)];
  }
  [[nodiscard]] LodColumn& at(int localX, int localZ) noexcept {
    return columns[static_cast<std::size_t>((localZ << 4) | localX)];
  }
};
[[nodiscard]] constexpr std::uint64_t packChunkKey(int chunkX, int chunkZ) noexcept {
  return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32) |
         static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkZ));
}
[[nodiscard]] constexpr int chunkKeyX(std::uint64_t key) noexcept {
  return static_cast<int>(static_cast<std::int32_t>(key >> 32));
}
[[nodiscard]] constexpr int chunkKeyZ(std::uint64_t key) noexcept {
  return static_cast<int>(static_cast<std::int32_t>(key & 0xFFFFFFFFULL));
}
[[nodiscard]] inline bool lodSurfaceBlock(int id) noexcept {
  if(id <= 0 || id >= net::minecraft::Block::BLOCK_COUNT) {
    return false;
  }
  if(id == 8 || id == 9) {
    return false;
  }
  if(id == 10 || id == 11 || id == 18 || id == 78 || id == 79) {
    return true;
  }
  return net::minecraft::Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(id)] > 0;
}
inline void extractLodChunk(const net::minecraft::Chunk& chunk, LodChunk& out) {
  for(int localZ = 0; localZ < 16; ++localZ) {
    for(int localX = 0; localX < 16; ++localX) {
      std::uint8_t waterTop = 0;
      std::uint8_t topY = 0;
      std::uint8_t topId = 0;
      for(int y = 127; y >= 0; --y) {
        const int id = chunk.getBlockId(localX, y, localZ);
        if(id == 0) {
          continue;
        }
        if(id == 8 || id == 9) {
          if(waterTop == 0) {
            waterTop = static_cast<std::uint8_t>(y);
          }
          continue;
        }
        if(lodSurfaceBlock(id)) {
          topY = static_cast<std::uint8_t>(y);
          topId = static_cast<std::uint8_t>(id);
          break;
        }
      }
      LodColumn& column = out.at(localX, localZ);
      column.topY = topY;
      column.topBlock = topId;
      column.waterY = waterTop > topY ? waterTop : 0;
      column.flags = 0;
    }
  }
  out.populated = chunk.terrainPopulated;
}
} // namespace net::minecraft::client::render::lod
