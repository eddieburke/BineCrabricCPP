#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
namespace net::minecraft {
class Chunk;
}
namespace net::minecraft::client::render::chunk {
// A private copy of the blocks and light a single mesh job reads, so the worker
// never races the main thread's edits.
//
// The copy is the shell the job actually reads -- the section plus one block on
// every side -- laid out as one flat column-major array. It used to be nine whole
// chunk bands, 48x48 columns held to reach an 18x18 footprint: seven eighths of
// every capture was copied and never read, and every accessor had to find which of
// the nine a coordinate fell in before it could index anything.
class RegionSnapshot final : public net::minecraft::BlockView {
 public:
 struct SourceChunk {
  int chunkX = 0;
  int chunkZ = 0;
  const net::minecraft::Chunk* chunk = nullptr;
 };
 RegionSnapshot(std::span<const SourceChunk> sourceChunks,
                int ambientDarkness,
                const std::array<float, 16>& lightLevelToLuminance,
                std::unique_ptr<net::minecraft::BiomeSource> biomeSource,
                int minBlockX,
                int minBlockY,
                int minBlockZ,
                int maxBlockX,
                int maxBlockY,
                int maxBlockZ);
 // Hands buffer_ back to the reuse pool instead of freeing it; see the pool in
 // RegionSnapshot.cpp for why the malloc/free pair was worth removing.
 ~RegionSnapshot() override;
 RegionSnapshot(const RegionSnapshot&) = delete;
 RegionSnapshot& operator=(const RegionSnapshot&) = delete;
 [[nodiscard]] int getBlockId(int x, int y, int z) const override;
 [[nodiscard]] net::minecraft::block::entity::BlockEntity* getBlockEntity(int, int, int) override {
  return nullptr;
 }
 [[nodiscard]] float getNaturalBrightness(int x, int y, int z, int blockLight) const override;
 [[nodiscard]] float getLightBrightness(int x, int y, int z) const override;
 [[nodiscard]] int getBlockMeta(int x, int y, int z) const override;
 [[nodiscard]] net::minecraft::block::material::Material& getMaterial(int x, int y, int z) const override;
 [[nodiscard]] bool isBlockOpaqueCube(int x, int y, int z) const override;
 [[nodiscard]] bool shouldSuffocate(int x, int y, int z) const override;
 [[nodiscard]] net::minecraft::BiomeSource* getBiomeSource() const override {
  return biomeSource_.get();
 }
 [[nodiscard]] int getRawBrightness(int x, int y, int z, bool useNeighborLight) const;
 [[nodiscard]] int getBlockLight(int x, int y, int z) const;
 [[nodiscard]] int getSkyLight(int x, int y, int z) const;
 [[nodiscard]] bool sawSkyLight() const noexcept { return sawSkyLight_; }
 [[nodiscard]] bool columnHasBlocks(int blockX, int blockZ, int minY, int maxY) const;
 // Captures that had to fall back to the chunk write lock. Per-cell writers are
 // outside the version protocol, so light traffic alone must never move this; the
 // perf harness asserts on it.
 [[nodiscard]] static std::uint64_t lockedCaptureCount() noexcept;
 // The five block-id columns the occlusion test reads: this one and its four
 // horizontal neighbours. Built once per (x,z) -- the mesher walks a section column
 // by column -- so the test is byte loads at fixed offsets.
 struct ColumnNeighbourhood {
  const std::uint8_t* self = nullptr;
  const std::uint8_t* negX = nullptr;
  const std::uint8_t* posX = nullptr;
  const std::uint8_t* negZ = nullptr;
  const std::uint8_t* posZ = nullptr;
  int minY = 0;
  int span = 0;
  bool complete = false;
  // The shell spans the section plus one block each side, so a section block's own
  // row is always in range; only y-1/y+1 at the world floor and ceiling are not,
  // which fullyEnclosed guards.
  [[nodiscard]] int rowFor(int y) const noexcept {
   return y - minY;
  }
  // Opaque with six opaque neighbours: nothing can show a face, so the mesher skips
  // it without paying render()'s per-block prelude. Opaque implies the solid layer,
  // so this can never hide geometry another layer owns.
  [[nodiscard]] bool fullyEnclosed(int row) const noexcept {
   if(!complete || row <= 0 || row + 1 >= span) {
    return false;
   }
   const bool* opaque = net::minecraft::block::Block::BLOCKS_OPAQUE.data();
   return opaque[self[row]] && opaque[self[row - 1]] && opaque[self[row + 1]] && opaque[negX[row]] &&
          opaque[posX[row]] && opaque[negZ[row]] && opaque[posZ[row]];
  }
 };
 [[nodiscard]] ColumnNeighbourhood columnNeighbourhood(int x, int z) const;

 private:
 // Column index into the shell, or -1 outside it. Column-major: a column's Y run is
 // contiguous, which is the order the mesher and the light sampler both walk.
 [[nodiscard]] int columnIndex(int x, int z) const noexcept {
  const int localX = x - minX_;
  const int localZ = z - minZ_;
  if(localX < 0 || localZ < 0 || localX >= spanX_ || localZ >= spanZ_) {
   return -1;
  }
  return localX * spanZ_ + localZ;
 }
 [[nodiscard]] bool containsY(int y) const noexcept {
  return y >= minY_ && y < minY_ + ySpan_;
 }
 [[nodiscard]] int nibbleAt(const std::uint8_t* bytes, int column, int y) const noexcept {
  const int row = y - minY_;
  const std::uint8_t byte = bytes[static_cast<std::size_t>(column * (ySpan_ >> 1) + (row >> 1))];
  return (row & 1) != 0 ? (byte >> 4U) & 0xF : byte & 0xF;
 }
 std::unique_ptr<std::uint8_t[]> buffer_;
 std::size_t bufferCapacity_ = 0;
 // Views into buffer_, in this order: block ids, metadata, sky light, block light.
 std::uint8_t* blocks_ = nullptr;
 std::uint8_t* meta_ = nullptr;
 std::uint8_t* skyLight_ = nullptr;
 std::uint8_t* blockLight_ = nullptr;
 int minX_ = 0;
 int minZ_ = 0;
 int spanX_ = 0;
 int spanZ_ = 0;
 int minY_ = 0;
 int ySpan_ = 0;
 int ambientDarkness_ = 0;
 std::array<float, 16> lightLevelToLuminance_{};
 std::unique_ptr<net::minecraft::BiomeSource> biomeSource_;
 mutable bool sawSkyLight_ = false;
};
} // namespace net::minecraft::client::render::chunk
