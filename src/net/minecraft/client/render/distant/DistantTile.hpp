#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>
#include "net/minecraft/client/gl/GlResource.hpp"
namespace net::minecraft {
class BiomeSource;
class Chunk;
}
namespace net::minecraft::client::texture {
struct RasterImage;
}
namespace net::minecraft::client::render::distant {
// see LOD_DESIGN.md §3, §5, §6
inline constexpr int kTileBlocks = 64;
inline constexpr int kTileChunks = kTileBlocks / 16;
inline constexpr int kMaxStride = 16;
inline constexpr int kNativeOverlapBlocks = 32;
// see third_party/iris/common/src/main/java/net/irisshaders/iris/pipeline/transform/transformer/DHTerrainTransformer.java
struct DhVertex {
 std::uint16_t x = 0;
 std::uint16_t y = 0;
 std::uint16_t z = 0;
 std::uint16_t meta = 0;
 std::uint8_t color[4]{};
 std::uint8_t material = 0;
 std::uint8_t normal = 0;
 std::uint8_t pad0 = 0;
 std::uint8_t pad1 = 0;
};
static_assert(sizeof(DhVertex) == 16);
enum DhMaterial : std::uint8_t {
 Unknown = 0,
 Leaves = 1,
 Stone = 2,
 Wood = 3,
 Metal = 4,
 Dirt = 5,
 Lava = 6,
 Deepslate = 7,
 Snow = 8,
 Sand = 9,
 Terracotta = 10,
 NetherStone = 11,
 Water = 12,
 Grass = 13,
 Air = 14,
 Illuminated = 15
};
enum DhNormal : std::uint8_t { NegY = 0, PosY = 1, NegZ = 2, PosZ = 3, NegX = 4, PosX = 5 };
inline constexpr int kMaxRuns = 4;
inline constexpr int kMaxMaterialLayers = 8;
struct ColumnMaterialLayer {
 std::uint8_t bottom = 0;
 std::uint8_t block = 0;
 std::uint8_t meta = 0;
};
// see LOD_DESIGN.md §3.2: runs are split only by sky-lit air or water, so sealed caves fill in.
struct ColumnRun {
 std::uint8_t top = 0;
 std::uint8_t bottom = 0;
 std::uint8_t block = 0;
 std::uint8_t meta = 0;
 std::uint8_t topLight = 0;
 std::uint8_t bottomLight = 0;
 std::array<ColumnMaterialLayer, kMaxMaterialLayers> layers{};
 std::uint8_t layerCount = 0;
};
struct ColumnCell {
 std::array<ColumnRun, kMaxRuns> runs{};
 std::uint8_t runCount = 0;
 std::uint8_t waterTop = 0;
 std::uint8_t waterLight = 0;
 std::uint8_t grass[3]{255, 255, 255};
 std::uint8_t foliage[3]{255, 255, 255};
 std::array<std::uint8_t, 128> light{};
 bool present = false;
};
struct TileCells {
 int tileX = 0;
 int tileZ = 0;
 int stride = 1;
 int side = kTileBlocks;
 std::uint16_t chunkMask = 0;
 std::vector<ColumnCell> cells;
 [[nodiscard]] ColumnCell& at(int cx, int cz) noexcept {
  return cells[static_cast<std::size_t>((cz + 1) * (side + 2) + (cx + 1))];
 }
 [[nodiscard]] const ColumnCell& at(int cx, int cz) const noexcept {
  return cells[static_cast<std::size_t>((cz + 1) * (side + 2) + (cx + 1))];
 }
};
using ChunkLookup = std::function<const Chunk*(int chunkX, int chunkZ)>;
struct PaletteEntry {
 std::uint8_t top[4]{};
 std::uint8_t side[4]{};
 std::uint8_t material = DhMaterial::Unknown;
 bool translucent = false;
 std::uint8_t tint = 0;
};
enum PaletteTint : std::uint8_t { NoTint = 0, GrassTint = 1, FoliageTint = 2 };
struct BlockPalette {
 std::array<PaletteEntry, 256> entries{};
};
[[nodiscard]] int tileStride(double distanceBlocks, double nativeBlocks, int quality, int memoryStride) noexcept;
[[nodiscard]] BlockPalette defaultPalette();
void updatePaletteFromAtlas(BlockPalette& palette, const client::texture::RasterImage& atlas);
[[nodiscard]] bool extractTile(const ChunkLookup& chunks,
                               int tileX,
                               int tileZ,
                               int stride,
                               TileCells& out,
                               const BiomeSource* biomes = nullptr,
                               bool allowPartial = false);
struct TileMesh {
 std::vector<DhVertex> opaque;
 std::vector<DhVertex> water;
 int minY = 0;
 int maxY = 0;
};
void meshTile(const TileCells& cells, const BlockPalette& palette, TileMesh& out);
class DistantSlab {
 public:
 static constexpr std::size_t kPageVertices = 256;
 struct Allocation {
  std::uint32_t firstPage = 0;
  std::uint32_t pageCount = 0;
  std::uint32_t vertexCount = 0;
  [[nodiscard]] std::uint32_t baseVertex() const noexcept {
   return firstPage * static_cast<std::uint32_t>(kPageVertices);
  }
 };
 bool create(std::size_t bytes);
 void destroy();
 [[nodiscard]] std::optional<Allocation> allocate(std::size_t vertexCount);
 void release(const Allocation& allocation);
 void upload(const Allocation& allocation, std::span<const DhVertex> vertices);
 [[nodiscard]] unsigned int vao() const noexcept;
 [[nodiscard]] std::size_t capacityBytes() const noexcept;
 [[nodiscard]] std::size_t usedBytes() const noexcept;

 private:
 gl::GlBuffer buffer_;
 gl::GlVao vao_;
 std::vector<std::pair<std::uint32_t, std::uint32_t>> freeRuns_;
 std::uint32_t pageCount_ = 0;
 std::uint32_t usedPages_ = 0;
};
} // namespace net::minecraft::client::render::distant
