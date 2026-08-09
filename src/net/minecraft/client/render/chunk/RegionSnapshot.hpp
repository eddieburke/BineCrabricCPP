#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <vector>
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

namespace net::minecraft {
class Chunk;
} // namespace net::minecraft

namespace net::minecraft::client::render::chunk {

class RegionSnapshot final : public net::minecraft::BlockView {
 public:
 struct SourceChunk {
  int chunkX = 0;
  int chunkZ = 0;
  const net::minecraft::Chunk* chunk = nullptr;
 };

 struct ChunkCopy {
  std::vector<std::uint8_t> storage;
  std::size_t blockBytes = 0;
  std::size_t nibbleBytes = 0;
  bool present = false;
  bool anyNonAir = false;
  [[nodiscard]] const std::uint8_t* blocksData() const noexcept { return storage.data(); }
  [[nodiscard]] const std::uint8_t* metaData() const noexcept { return storage.data() + blockBytes; }
  [[nodiscard]] const std::uint8_t* skyLightData() const noexcept { return storage.data() + blockBytes + nibbleBytes; }
  [[nodiscard]] const std::uint8_t* blockLightData() const noexcept { return storage.data() + blockBytes + 2 * nibbleBytes; }
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
 [[nodiscard]] bool sawSkyLight() const noexcept {
  return sawSkyLight_;
 }
 [[nodiscard]] bool columnHasBlocks(int blockX, int blockZ, int minY, int maxY) const;

 private:
 [[nodiscard]] const ChunkCopy* chunkAt(int x, int z) const {
  const int localX = (x >> 4) - chunkX_;
  const int localZ = (z >> 4) - chunkZ_;
  // Every block, meta and light query resolves its chunk this way, and a single
  // getRawBrightness with neighbour sampling asks eleven times — almost always
  // for the same chunk, since the five neighbours of a block share its column
  // except at a 16-block boundary. Memoise the last answer.
  if(localX == memoX_ && localZ == memoZ_) {
   return memo_;
  }
  const ChunkCopy* result = nullptr;
  if(localX >= 0 && localZ >= 0 && localX < chunkWidth_ && localZ < chunkDepth_) {
   const ChunkCopy& copy = chunks_[static_cast<std::size_t>(localX + localZ * chunkWidth_)];
   result = copy.present ? &copy : nullptr;
  }
  memoX_ = localX;
  memoZ_ = localZ;
  memo_ = result;
  return result;
 }
 [[nodiscard]] bool containsY(int y) const noexcept {
  return y >= minY_ && y < minY_ + ySpan_;
 }
 [[nodiscard]] std::size_t snapshotIndex(int localX, int y, int localZ) const noexcept {
  return static_cast<std::size_t>(((localX << 4) | localZ) * ySpan_ + (y - minY_));
 }
 [[nodiscard]] int nibbleAt(const std::uint8_t* bytes, int localX, int y, int localZ) const noexcept {
  const std::size_t byteIndex =
      static_cast<std::size_t>(((localX << 4) | localZ) * (ySpan_ >> 1) + ((y - minY_) >> 1));
  const std::uint8_t byte = bytes[byteIndex];
  return ((y - minY_) & 1) != 0 ? (byte >> 4U) & 0xF : byte & 0xF;
 }
 int chunkX_ = 0;
 int chunkZ_ = 0;
 int chunkWidth_ = 0;
 int chunkDepth_ = 0;
 int minY_ = 0;
 int ySpan_ = 0;
 std::vector<ChunkCopy> chunks_;
 int ambientDarkness_ = 0;
 std::array<float, 16> lightLevelToLuminance_{};
 std::unique_ptr<net::minecraft::BiomeSource> biomeSource_;
 mutable bool sawSkyLight_ = false;
 // chunkAt memo. Seeded out of range so the first lookup always misses. Not
 // synchronised — a snapshot belongs to the one mesh job that built it, same
 // assumption sawSkyLight_ already makes.
 mutable int memoX_ = std::numeric_limits<int>::min();
 mutable int memoZ_ = std::numeric_limits<int>::min();
 mutable const ChunkCopy* memo_ = nullptr;
};

} // namespace net::minecraft::client::render::chunk
