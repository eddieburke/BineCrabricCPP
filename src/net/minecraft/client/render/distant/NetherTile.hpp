#pragma once
#include "net/minecraft/client/render/distant/DistantTile.hpp"

namespace net::minecraft::client::render::distant {
inline constexpr int kNetherHeight = 128;
struct NetherColumn {
 std::array<std::uint8_t, kNetherHeight> blocks{};
 std::array<std::uint8_t, kNetherHeight> light{};
 bool present = false;
 bool operator==(const NetherColumn&) const = default;
};
struct NetherTileCells {
 int stride = 1;
 int side = kTileBlocks;
 std::uint16_t chunkMask = 0;
 std::vector<NetherColumn> columns;
 [[nodiscard]] NetherColumn& at(int x, int z) noexcept {
  return columns[static_cast<std::size_t>((z + 1) * (side + 2) + x + 1)];
 }
 [[nodiscard]] const NetherColumn& at(int x, int z) const noexcept {
  return columns[static_cast<std::size_t>((z + 1) * (side + 2) + x + 1)];
 }
};
[[nodiscard]] bool extractNetherTile(const ChunkLookup& chunks, int tileX, int tileZ, int stride,
                                    NetherTileCells& out, bool allowPartial = false);
void meshNetherTile(const NetherTileCells& cells, const BlockPalette& palette, TileMesh& out);
}
