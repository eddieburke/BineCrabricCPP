#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft::client::render::chunk {
namespace {
// Every mesh capture allocated a fresh ~100 KB snapshot buffer and freed it again
// once the worker was done -- on the main thread, several times a frame, and it was
// the single largest allocator cost in the frame. The buffer holds nothing but bytes
// the constructor overwrites in full, so a finished one can simply be handed back.
struct PooledBuffer {
 std::unique_ptr<std::uint8_t[]> bytes;
 std::size_t capacity = 0;
};
constexpr std::size_t kMaxPooledBuffers = 32;
std::mutex gPoolMutex;
std::vector<PooledBuffer> gPooledBuffers;
[[nodiscard]] PooledBuffer acquireSnapshotBuffer(std::size_t wanted) {
 {
  const std::lock_guard<std::mutex> guard(gPoolMutex);
  for(std::size_t index = 0; index < gPooledBuffers.size(); ++index) {
   if(gPooledBuffers[index].capacity < wanted) {
    continue;
   }
   PooledBuffer taken = std::move(gPooledBuffers[index]);
   gPooledBuffers[index] = std::move(gPooledBuffers.back());
   gPooledBuffers.pop_back();
   return taken;
  }
 }
 return PooledBuffer{std::make_unique_for_overwrite<std::uint8_t[]>(wanted), wanted};
}
void releaseSnapshotBuffer(PooledBuffer buffer) {
 if(buffer.bytes == nullptr) {
  return;
 }
 const std::lock_guard<std::mutex> guard(gPoolMutex);
 // Over the cap the buffer just falls off the end of this scope and is freed, so
 // a view-distance change cannot pin an unbounded pile of oversized buffers.
 if(gPooledBuffers.size() < kMaxPooledBuffers) {
  gPooledBuffers.push_back(std::move(buffer));
 }
}
class ChunkRenderWriteLock {
 public:
 explicit ChunkRenderWriteLock(const Chunk& chunk) : chunk_(chunk) {
  chunk_.lockRenderWrite();
 }
 ~ChunkRenderWriteLock() {
  chunk_.unlockRenderWrite();
 }
 ChunkRenderWriteLock(const ChunkRenderWriteLock&) = delete;
 ChunkRenderWriteLock& operator=(const ChunkRenderWriteLock&) = delete;

 private:
 const Chunk& chunk_;
};
void copyNibbleColumn(std::uint8_t* dst,
                      std::size_t dstColumn,
                      int halfSpan,
                      const std::vector<std::uint8_t>& src,
                      std::size_t srcByteBase) {
 std::uint8_t* out = dst + dstColumn * static_cast<std::size_t>(halfSpan);
 if(srcByteBase + static_cast<std::size_t>(halfSpan) <= src.size()) {
  std::memcpy(out, src.data() + srcByteBase, static_cast<std::size_t>(halfSpan));
  return;
 }
 for(int i = 0; i < halfSpan; ++i) {
  const std::size_t srcIdx = srcByteBase + static_cast<std::size_t>(i);
  if(srcIdx < src.size()) {
   out[i] = src[srcIdx];
  } else {
   out[i] = 0;
  }
 }
}
void copyChunkBand(std::uint8_t* dst,
                   const Chunk& chunk,
                   int minY,
                   int ySpan,
                   int halfSpan,
                   bool& anyNonAir) {
 ChunkRenderWriteLock guard(chunk);
 anyNonAir = false;
 const std::size_t blockBytes = static_cast<std::size_t>(16 * 16 * ySpan);
 const std::size_t nibbleBytes = static_cast<std::size_t>(16 * 16 * halfSpan);
 std::uint8_t* blocks = dst;
 std::uint8_t* meta = blocks + blockBytes;
 std::uint8_t* skyLight = meta + nibbleBytes;
 std::uint8_t* blockLight = skyLight + nibbleBytes;
 const std::size_t blockNeed = static_cast<std::size_t>((15 << 11) | (15 << 7) | minY) + static_cast<std::size_t>(ySpan);
 if(chunk.blocks.size() < blockNeed) {
  std::memset(dst, 0, blockBytes + 3 * nibbleBytes);
  return;
 }
 // The three nibble arrays are sized per chunk, so the per-column bounds test
 // copyNibbleColumn does is loop-invariant; hoisting it lets the common case
 // memcpy 256 columns without reloading three vector sizes each time.
 const std::size_t nibbleNeed =
     (static_cast<std::size_t>((15 << 11) | (15 << 7) | minY) >> 1) + static_cast<std::size_t>(halfSpan);
 const bool nibblesInRange = chunk.meta.bytes.size() >= nibbleNeed &&
                             chunk.skyLight.bytes.size() >= nibbleNeed &&
                             chunk.blockLight.bytes.size() >= nibbleNeed;
 for(int column = 0; column < 16 * 16; ++column) {
  const int localX = column >> 4;
  const int localZ = column & 0xF;
  const std::size_t srcBlockBase = static_cast<std::size_t>((localX << 11) | (localZ << 7) | minY);
  const std::size_t srcNibbleBase = srcBlockBase >> 1;
  const std::size_t dstColumn = static_cast<std::size_t>((localX << 4) | localZ);
  std::memcpy(blocks + dstColumn * static_cast<std::size_t>(ySpan),
              chunk.blocks.data() + srcBlockBase,
              static_cast<std::size_t>(ySpan));
  if(!anyNonAir) {
   for(int i = 0; i < ySpan; ++i) {
    if(blocks[dstColumn * static_cast<std::size_t>(ySpan) + static_cast<std::size_t>(i)] != 0) {
     anyNonAir = true;
     break;
    }
   }
  }
  if(nibblesInRange) {
   const std::size_t dstByte = dstColumn * static_cast<std::size_t>(halfSpan);
   const std::size_t bytes = static_cast<std::size_t>(halfSpan);
   std::memcpy(meta + dstByte, chunk.meta.bytes.data() + srcNibbleBase, bytes);
   std::memcpy(skyLight + dstByte, chunk.skyLight.bytes.data() + srcNibbleBase, bytes);
   std::memcpy(blockLight + dstByte, chunk.blockLight.bytes.data() + srcNibbleBase, bytes);
  } else {
   copyNibbleColumn(meta, dstColumn, halfSpan, chunk.meta.bytes, srcNibbleBase);
   copyNibbleColumn(skyLight, dstColumn, halfSpan, chunk.skyLight.bytes, srcNibbleBase);
   copyNibbleColumn(blockLight, dstColumn, halfSpan, chunk.blockLight.bytes, srcNibbleBase);
  }
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
 blockBytesPerChunk_ = static_cast<std::size_t>(16 * 16 * ySpan_);
 nibbleBytesPerChunk_ = static_cast<std::size_t>(16 * 16 * halfSpan);
 const std::size_t perChunkBytes = blockBytesPerChunk_ + 3 * nibbleBytesPerChunk_;
 const std::size_t totalBytes = perChunkBytes * sourceChunks.size();
 // copyChunkBand fully overwrites every present chunk's span, so the buffer is
 // taken without the value-init memset resize() would do; absent chunks are
 // zeroed below so their bytes stay deterministic. That same "written in full"
 // property is what makes a recycled buffer safe to take here.
 PooledBuffer taken = acquireSnapshotBuffer(totalBytes);
 buffer_ = std::move(taken.bytes);
 bufferCapacity_ = taken.capacity;
 std::size_t offset = 0;
 for(const SourceChunk& source : sourceChunks) {
  if(source.chunk == nullptr || source.chunkX < chunkX_ || source.chunkZ < chunkZ_ ||
     source.chunkX > maxChunkX || source.chunkZ > maxChunkZ) {
   std::memset(buffer_.get() + offset, 0, perChunkBytes);
   offset += perChunkBytes;
   continue;
  }
  const Chunk& chunk = *source.chunk;
  ChunkInfo& info = chunks_[static_cast<std::size_t>((source.chunkX - chunkX_) + (source.chunkZ - chunkZ_) * chunkWidth_)];
  info.offset = static_cast<std::uint32_t>(offset);
  info.present = true;
  copyChunkBand(buffer_.get() + offset, chunk, minY_, ySpan_, halfSpan, info.anyNonAir);
  offset += perChunkBytes;
 }
 ambientDarkness_ = ambientDarkness;
 lightLevelToLuminance_ = lightLevelToLuminance;
 biomeSource_ = std::move(biomeSource);
}
RegionSnapshot::~RegionSnapshot() {
 releaseSnapshotBuffer({std::move(buffer_), bufferCapacity_});
}
bool RegionSnapshot::columnHasBlocks(int blockX, int blockZ, int minY, int maxY) const {
 const ChunkInfo* info = chunkInfo(blockX, blockZ);
 if(info == nullptr || !info->anyNonAir) {
  return false;
 }
 const int clampedMinY = std::max(minY, minY_);
 const int clampedMaxY = std::min(maxY, minY_ + ySpan_);
 if(clampedMinY >= clampedMaxY) {
  return false;
 }
 const std::size_t spanBytes = static_cast<std::size_t>(clampedMaxY - clampedMinY);
 constexpr std::size_t kWordBytes = sizeof(std::uint64_t);
 const std::uint8_t* blocks = chunkBlocks(*info);
 for(int column = 0; column < 16 * 16; ++column) {
  const std::uint8_t* base =
      blocks + static_cast<std::size_t>(column * ySpan_ + (clampedMinY - minY_));
  std::size_t i = 0;
  for(; i + kWordBytes <= spanBytes; i += kWordBytes) {
   std::uint64_t word = 0;
   std::memcpy(&word, base + i, kWordBytes);
   if(word != 0) {
    return true;
   }
  }
  for(; i < spanBytes; ++i) {
   if(base[i] != 0) {
    return true;
   }
  }
 }
 return false;
}
int RegionSnapshot::getBlockId(int x, int y, int z) const {
 if(y < 0 || y >= Chunk::height) {
  return 0;
 }
 const ChunkInfo* info = chunkInfo(x, z);
 if(info == nullptr || !containsY(y)) {
  return 0;
 }
 return static_cast<int>(chunkBlocks(*info)[snapshotIndex(x & 0xF, y, z & 0xF)] & 0xFFU);
}
int RegionSnapshot::getBlockMeta(int x, int y, int z) const {
 if(y < 0 || y >= Chunk::height) {
  return 0;
 }
 const ChunkInfo* info = chunkInfo(x, z);
 if(info == nullptr || !containsY(y)) {
  return 0;
 }
 return nibbleAt(chunkMeta(*info), x & 0xF, y, z & 0xF);
}
float RegionSnapshot::getNaturalBrightness(int x, int y, int z, int blockLight) const {
 int brightness = getRawBrightness(x, y, z, true);
 if(brightness < blockLight) {
  brightness = blockLight;
 }
 return lightLevelToLuminance_[static_cast<std::size_t>(brightness)];
}
float RegionSnapshot::getLightBrightness(int x, int y, int z) const {
 return lightLevelToLuminance_[static_cast<std::size_t>(getRawBrightness(x, y, z, true))];
}
int RegionSnapshot::getRawBrightness(int x, int y, int z, bool useNeighborLight) const {
 if(y < 0) {
  return 0;
 }
 if(y >= Chunk::height) {
  const int brightness = 15 - ambientDarkness_;
  return brightness < 0 ? 0 : brightness;
 }
 const ChunkInfo* info = chunkInfo(x, z);
 if(info == nullptr || !containsY(y)) {
  const int brightness = 15 - ambientDarkness_;
  return brightness < 0 ? 0 : brightness;
 }
 if(useNeighborLight && block::Block::usesNeighborLightSampling(getBlockId(x, y, z))) {
  int brightness = getRawBrightness(x, y + 1, z, false);
  brightness = std::max(brightness, getRawBrightness(x + 1, y, z, false));
  brightness = std::max(brightness, getRawBrightness(x - 1, y, z, false));
  brightness = std::max(brightness, getRawBrightness(x, y, z + 1, false));
  brightness = std::max(brightness, getRawBrightness(x, y, z - 1, false));
  return brightness;
 }
 int sky = nibbleAt(chunkSkyLight(*info), x & 0xF, y, z & 0xF);
 if(sky > 0) {
  sawSkyLight_ = true;
 }
 const int block = nibbleAt(chunkBlockLight(*info), x & 0xF, y, z & 0xF);
 if(block > (sky -= ambientDarkness_)) {
  sky = block;
 }
 return sky < 0 ? 0 : sky;
}
int RegionSnapshot::getBlockLight(const int x, const int y, const int z) const {
 if(y < 0 || y >= Chunk::height) {
  return 0;
 }
 const ChunkInfo* info = chunkInfo(x, z);
 if(info == nullptr || !containsY(y)) {
  return 0;
 }
 return nibbleAt(chunkBlockLight(*info), x & 0xF, y, z & 0xF);
}
int RegionSnapshot::getSkyLight(const int x, const int y, const int z) const {
 if(y < 0) {
  return 0;
 }
 if(y >= Chunk::height) {
  return 15;
 }
 const ChunkInfo* info = chunkInfo(x, z);
 if(info == nullptr || !containsY(y)) {
  return 15;
 }
 int sky = nibbleAt(chunkSkyLight(*info), x & 0xF, y, z & 0xF);
 if(sky > 0) {
  sawSkyLight_ = true;
 }
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
