#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft::client::render::chunk {
namespace {
[[nodiscard]] std::size_t minBlockBytesForBand(int minY, int ySpan) noexcept {
  return static_cast<std::size_t>((15 << 11) | (15 << 7) | minY) + static_cast<std::size_t>(ySpan);
}
void copyNibbleColumn(std::uint8_t* dst,
                      std::size_t dstColumn,
                      int halfSpan,
                      const std::vector<std::uint8_t>& src,
                      std::size_t srcByteBase,
                      bool atomicRead) {
  std::uint8_t* out = dst + dstColumn * static_cast<std::size_t>(halfSpan);
  if(!atomicRead && srcByteBase + static_cast<std::size_t>(halfSpan) <= src.size()) {
    std::memcpy(out, src.data() + srcByteBase, static_cast<std::size_t>(halfSpan));
    return;
  }
  for(int i = 0; i < halfSpan; ++i) {
    const std::size_t srcIdx = srcByteBase + static_cast<std::size_t>(i);
    if(srcIdx < src.size()) {
      out[i] = atomicRead
                   ? std::atomic_ref(const_cast<std::uint8_t&>(src[srcIdx])).load(std::memory_order_relaxed)
                   : src[srcIdx];
    } else {
      out[i] = 0;
    }
  }
}
void copyChunkBand(std::vector<std::uint8_t>& blocks,
                   std::vector<std::uint8_t>& meta,
                   std::vector<std::uint8_t>& skyLight,
                   std::vector<std::uint8_t>& blockLight,
                   const Chunk& chunk,
                   int minY,
                   int ySpan,
                   int halfSpan,
                   std::size_t blockBytes,
                   std::size_t nibbleBytes) {
  const std::size_t blockNeed = minBlockBytesForBand(minY, ySpan);
  if(chunk.blocks.size() < blockNeed) {
    blocks.assign(blockBytes, 0);
    meta.assign(nibbleBytes, 0);
    skyLight.assign(nibbleBytes, 0);
    blockLight.assign(nibbleBytes, 0);
    return;
  }
  blocks.resize(blockBytes);
  meta.resize(nibbleBytes);
  skyLight.resize(nibbleBytes);
  blockLight.resize(nibbleBytes);
  for(int column = 0; column < 16 * 16; ++column) {
    const int localX = column >> 4;
    const int localZ = column & 0xF;
    const std::size_t srcBlockBase = static_cast<std::size_t>((localX << 11) | (localZ << 7) | minY);
    const std::size_t srcNibbleBase = srcBlockBase >> 1;
    const std::size_t dstColumn = static_cast<std::size_t>((localX << 4) | localZ);
    std::memcpy(blocks.data() + dstColumn * static_cast<std::size_t>(ySpan),
                chunk.blocks.data() + srcBlockBase,
                static_cast<std::size_t>(ySpan));
    copyNibbleColumn(meta.data(), dstColumn, halfSpan, chunk.meta.bytes, srcNibbleBase, false);
    copyNibbleColumn(skyLight.data(), dstColumn, halfSpan, chunk.skyLight.bytes, srcNibbleBase, true);
    copyNibbleColumn(blockLight.data(), dstColumn, halfSpan, chunk.blockLight.bytes, srcNibbleBase, true);
  }
}
} // namespace
RegionSnapshot::RegionSnapshot(std::span<const SourceChunk> sourceChunks,
                               int ambientDarkness,
                               const std::array<float, 16>& lightLevelToLuminance,
                               std::unique_ptr<net::minecraft::BiomeSource> biomeSource,
                               int minBlockX,
                               int minBlockY,
                               int minBlockZ,
                               int maxBlockX,
                               int maxBlockY,
                               int maxBlockZ) {
  chunkX_ = minBlockX >> 4;
  chunkZ_ = minBlockZ >> 4;
  const int maxChunkX = maxBlockX >> 4;
  const int maxChunkZ = maxBlockZ >> 4;
  chunkWidth_ = maxChunkX - chunkX_ + 1;
  chunkDepth_ = maxChunkZ - chunkZ_ + 1;
  chunks_.resize(static_cast<std::size_t>(chunkWidth_ * chunkDepth_));
  minY_ = std::clamp(minBlockY, 0, Chunk::height - 1) & ~1;
  const int maxY = std::clamp(maxBlockY, 0, Chunk::height - 1);
  int span = maxY >= minY_ ? maxY - minY_ + 1 : 0;
  span += span & 1;
  ySpan_ = std::min(span, Chunk::height - minY_);
  const int halfSpan = ySpan_ >> 1;
  const std::size_t blockBytes = static_cast<std::size_t>(16 * 16 * ySpan_);
  const std::size_t nibbleBytes = static_cast<std::size_t>(16 * 16 * halfSpan);
  for(const SourceChunk& source : sourceChunks) {
    if(source.chunk == nullptr || source.chunkX < chunkX_ || source.chunkZ < chunkZ_ ||
       source.chunkX > maxChunkX || source.chunkZ > maxChunkZ) {
      continue;
    }
    const Chunk& chunk = *source.chunk;
    ChunkCopy& copy =
        chunks_[static_cast<std::size_t>((source.chunkX - chunkX_) + (source.chunkZ - chunkZ_) * chunkWidth_)];
    copy.present = true;
    copyChunkBand(copy.blocks,
                  copy.meta,
                  copy.skyLight,
                  copy.blockLight,
                  chunk,
                  minY_,
                  ySpan_,
                  halfSpan,
                  blockBytes,
                  nibbleBytes);
  }
  ambientDarkness_ = ambientDarkness;
  lightLevelToLuminance_ = lightLevelToLuminance;
  biomeSource_ = std::move(biomeSource);
}
int RegionSnapshot::getBlockId(int x, int y, int z) const {
  if(y < 0 || y >= Chunk::height) {
    return 0;
  }
  const ChunkCopy* chunk = chunkAt(x, z);
  if(chunk == nullptr || !containsY(y)) {
    return 0;
  }
  return static_cast<int>(chunk->blocks[snapshotIndex(x & 0xF, y, z & 0xF)] & 0xFFU);
}
int RegionSnapshot::getBlockMeta(int x, int y, int z) const {
  if(y < 0 || y >= Chunk::height) {
    return 0;
  }
  const ChunkCopy* chunk = chunkAt(x, z);
  if(chunk == nullptr || !containsY(y)) {
    return 0;
  }
  return nibbleAt(chunk->meta, x & 0xF, y, z & 0xF);
}
float RegionSnapshot::getNaturalBrightness(int x, int y, int z, int blockLight) const {
  int brightness = getRawBrightness(x, y, z);
  if(brightness < blockLight) {
    brightness = blockLight;
  }
  return lightLevelToLuminance_[static_cast<std::size_t>(brightness)];
}
float RegionSnapshot::getLightBrightness(int x, int y, int z) const {
  return lightLevelToLuminance_[static_cast<std::size_t>(getRawBrightness(x, y, z))];
}
int RegionSnapshot::getRawBrightness(int x, int y, int z, bool useNeighborLight) const {
  if(x < -32000000 || z < -32000000 || x >= 32000000 || z > 32000000) {
    return 15;
  }
  if(useNeighborLight && block::Block::usesNeighborLightSampling(getBlockId(x, y, z))) {
    int brightness = getRawBrightness(x, y + 1, z, false);
    brightness = std::max(brightness, getRawBrightness(x + 1, y, z, false));
    brightness = std::max(brightness, getRawBrightness(x - 1, y, z, false));
    brightness = std::max(brightness, getRawBrightness(x, y, z + 1, false));
    brightness = std::max(brightness, getRawBrightness(x, y, z - 1, false));
    return brightness;
  }
  if(y < 0) {
    return 0;
  }
  if(y >= Chunk::height) {
    const int brightness = 15 - ambientDarkness_;
    return brightness < 0 ? 0 : brightness;
  }
  const ChunkCopy* chunk = chunkAt(x, z);
  if(chunk == nullptr || !containsY(y)) {
    return 0;
  }
  // Mirrors Chunk::getLight(localX, y, localZ, ambientDarkness), with the
  // skylight detection recorded per-snapshot instead of a global static.
  int sky = nibbleAt(chunk->skyLight, x & 0xF, y, z & 0xF);
  if(sky > 0) {
    sawSkyLight_ = true;
  }
  const int block = nibbleAt(chunk->blockLight, x & 0xF, y, z & 0xF);
  if(block > (sky -= ambientDarkness_)) {
    sky = block;
  }
  return sky < 0 ? 0 : sky;
}
int RegionSnapshot::getBlockLight(const int x, const int y, const int z) const {
  if(y < 0 || y >= Chunk::height) {
    return 0;
  }
  const ChunkCopy* chunk = chunkAt(x, z);
  if(chunk == nullptr || !containsY(y)) {
    return 0;
  }
  return nibbleAt(chunk->blockLight, x & 0xF, y, z & 0xF);
}
int RegionSnapshot::getSkyLight(const int x, const int y, const int z) const {
  if(y < 0) {
    return 0;
  }
  if(y >= Chunk::height) {
    const int brightness = 15 - ambientDarkness_;
    return brightness < 0 ? 0 : brightness;
  }
  const ChunkCopy* chunk = chunkAt(x, z);
  if(chunk == nullptr || !containsY(y)) {
    return 0;
  }
  int sky = nibbleAt(chunk->skyLight, x & 0xF, y, z & 0xF);
  if(sky > 0) {
    sawSkyLight_ = true;
  }
  sky -= ambientDarkness_;
  return sky < 0 ? 0 : sky;
}
net::minecraft::block::material::Material& RegionSnapshot::getMaterial(int x, int y, int z) const {
  const int blockId = getBlockId(x, y, z);
  if(blockId == 0) {
    return block::material::Material::AIR;
  }
  block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
  if(block == nullptr) {
    return block::material::Material::AIR;
  }
  return block->material;
}
bool RegionSnapshot::isBlockOpaqueCube(int x, int y, int z) const {
  block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(getBlockId(x, y, z))];
  if(block == nullptr) {
    return false;
  }
  return block->isOpaque();
}
bool RegionSnapshot::shouldSuffocate(int x, int y, int z) const {
  block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(getBlockId(x, y, z))];
  if(block == nullptr) {
    return false;
  }
  return block->material.blocksMovement() && block->isFullCube();
}
} // namespace net::minecraft::client::render::chunk
