#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include <thread>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft::client::render::chunk {
namespace {
// Every mesh capture allocated a fresh snapshot buffer and freed it again once the
// worker was done -- on the main thread, several times a frame, and it was the
// single largest allocator cost in the frame. The buffer holds nothing but bytes
// the constructor overwrites in full, so a finished one can simply be handed back.
struct PooledBuffer {
 std::unique_ptr<std::uint8_t[]> bytes;
 std::size_t capacity = 0;
};
constexpr std::size_t kMaxPooledBuffers = 32;
std::mutex gPoolMutex;
std::vector<PooledBuffer> gPooledBuffers;
std::atomic<std::uint64_t> gLockedCaptures{0};
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
// One column of the shell: the Y run of block ids, metadata, sky light and block
// light for a single (x,z), copied straight out of the chunk's own arrays. Both
// layouts are column-major with y contiguous, and the shell's minY is forced even
// so the packed nibble runs line up byte for byte -- every one of these is a
// memcpy, never a shift.
struct ShellColumn {
 std::uint8_t* blocks;
 std::uint8_t* meta;
 std::uint8_t* skyLight;
 std::uint8_t* blockLight;
};
void copyColumnOnce(const ShellColumn& dst,
                    const Chunk& chunk,
                    int localX,
                    int localZ,
                    int minY,
                    int ySpan,
                    int halfSpan) {
 const std::size_t blockBase = static_cast<std::size_t>((localX << 11) | (localZ << 7) | minY);
 const std::size_t nibbleBase = blockBase >> 1;
 const auto blockBytes = static_cast<std::size_t>(ySpan);
 const auto nibbleBytes = static_cast<std::size_t>(halfSpan);
 if(chunk.blocks.size() < blockBase + blockBytes) {
  std::memset(dst.blocks, 0, blockBytes);
 } else {
  std::memcpy(dst.blocks, chunk.blocks.data() + blockBase, blockBytes);
 }
 const auto copyNibbles = [nibbleBase, nibbleBytes](std::uint8_t* out, const std::vector<std::uint8_t>& src) {
  if(src.size() < nibbleBase + nibbleBytes) {
   std::memset(out, 0, nibbleBytes);
   return;
  }
  std::memcpy(out, src.data() + nibbleBase, nibbleBytes);
 };
 copyNibbles(dst.meta, chunk.meta.bytes);
 copyNibbles(dst.skyLight, chunk.skyLight.bytes);
 copyNibbles(dst.blockLight, chunk.blockLight.bytes);
}
void zeroColumn(const ShellColumn& dst, int ySpan, int halfSpan) {
 std::memset(dst.blocks, 0, static_cast<std::size_t>(ySpan));
 std::memset(dst.meta, 0, static_cast<std::size_t>(halfSpan));
 std::memset(dst.skyLight, 0, static_cast<std::size_t>(halfSpan));
 std::memset(dst.blockLight, 0, static_cast<std::size_t>(halfSpan));
}
// The capture runs on the main thread inside terrain/compile, and every chunk it
// touches is one a light worker may be writing. Taking the chunk's write lock here
// made looking around a hitch: each acquisition waited behind whichever worker held
// the flag, and every light cell that worker wrote spun behind us in turn.
//
// So read optimistically instead. Per-cell writes are byte-atomic and are not in the
// version protocol at all: racing one yields a stale value, never a torn one, and the
// write that produced it publishes a dirty region that remeshes the section anyway.
// Only a bulk section -- packet load, save serialise -- can leave the bytes actually
// inconsistent, and those are exactly what an odd stamp reports. A capture that keeps
// landing inside one falls back to the lock so it cannot spin forever.
constexpr int kOptimisticAttempts = 4;
template <typename CopyRect>
void captureChunkRect(const Chunk& chunk, CopyRect&& copyRect) {
 for(int attempt = 0; attempt < kOptimisticAttempts; ++attempt) {
  const std::uint32_t stamp = chunk.renderVersion();
  if((stamp & 1U) != 0) {
   // A bulk section is in flight. Copying now would only be thrown away.
   std::this_thread::yield();
   continue;
  }
  copyRect();
  if(chunk.renderReadValid(stamp)) {
   return;
  }
 }
 gLockedCaptures.fetch_add(1, std::memory_order_relaxed);
 const Chunk::RenderWriteGuard guard(chunk);
 copyRect();
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
 minX_ = minBlockX;
 minZ_ = minBlockZ;
 spanX_ = std::max(maxBlockX - minBlockX + 1, 0);
 spanZ_ = std::max(maxBlockZ - minBlockZ + 1, 0);
 // Even minY and even span keep every nibble run byte-aligned against the chunk's,
 // so the light copies stay memcpys.
 minY_ = std::clamp(minBlockY, 0, Chunk::height - 1) & ~1;
 const int maxY = std::clamp(maxBlockY, 0, Chunk::height - 1);
 int span = maxY >= minY_ ? maxY - minY_ + 1 : 0;
 span += span & 1;
 ySpan_ = std::min(span, Chunk::height - minY_);
 const int halfSpan = ySpan_ >> 1;
 const auto columns = static_cast<std::size_t>(spanX_ * spanZ_);
 const std::size_t blockBytes = columns * static_cast<std::size_t>(ySpan_);
 const std::size_t nibbleBytes = columns * static_cast<std::size_t>(halfSpan);
 // Every column is written in full below -- copied or zeroed -- so the buffer is
 // taken without the value-init a resize() would do. That same "written in full"
 // property is what makes a recycled buffer safe to take here.
 PooledBuffer taken = acquireSnapshotBuffer(blockBytes + 3 * nibbleBytes);
 buffer_ = std::move(taken.bytes);
 bufferCapacity_ = taken.capacity;
 blocks_ = buffer_.get();
 meta_ = blocks_ + blockBytes;
 skyLight_ = meta_ + nibbleBytes;
 blockLight_ = skyLight_ + nibbleBytes;
 const auto columnAt = [this, halfSpan](int column) {
  return ShellColumn{blocks_ + static_cast<std::size_t>(column * ySpan_),
                     meta_ + static_cast<std::size_t>(column * halfSpan),
                     skyLight_ + static_cast<std::size_t>(column * halfSpan),
                     blockLight_ + static_cast<std::size_t>(column * halfSpan)};
 };
 // Walk the chunks rather than the columns: the seqlock stamp has to bracket a
 // whole chunk's worth of copying for its retry to mean anything.
 const int minChunkX = minX_ >> 4;
 const int maxChunkX = (minX_ + spanX_ - 1) >> 4;
 const int minChunkZ = minZ_ >> 4;
 const int maxChunkZ = (minZ_ + spanZ_ - 1) >> 4;
 for(int chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX) {
  for(int chunkZ = minChunkZ; chunkZ <= maxChunkZ; ++chunkZ) {
   const Chunk* chunk = nullptr;
   for(const SourceChunk& source : sourceChunks) {
    if(source.chunkX == chunkX && source.chunkZ == chunkZ) {
     chunk = source.chunk;
     break;
    }
   }
   const int fromX = std::max(minX_, chunkX * 16);
   const int toX = std::min(minX_ + spanX_ - 1, chunkX * 16 + 15);
   const int fromZ = std::max(minZ_, chunkZ * 16);
   const int toZ = std::min(minZ_ + spanZ_ - 1, chunkZ * 16 + 15);
   if(fromX > toX || fromZ > toZ) {
    continue;
   }
   if(chunk == nullptr) {
    for(int x = fromX; x <= toX; ++x) {
     for(int z = fromZ; z <= toZ; ++z) {
      zeroColumn(columnAt(columnIndex(x, z)), ySpan_, halfSpan);
     }
    }
    continue;
   }
   captureChunkRect(*chunk, [&] {
    for(int x = fromX; x <= toX; ++x) {
     for(int z = fromZ; z <= toZ; ++z) {
      copyColumnOnce(columnAt(columnIndex(x, z)), *chunk, x & 0xF, z & 0xF, minY_, ySpan_, halfSpan);
     }
    }
   });
  }
 }
 ambientDarkness_ = ambientDarkness;
 lightLevelToLuminance_ = lightLevelToLuminance;
 biomeSource_ = std::move(biomeSource);
}
std::uint64_t RegionSnapshot::lockedCaptureCount() noexcept {
 return gLockedCaptures.load(std::memory_order_relaxed);
}
RegionSnapshot::~RegionSnapshot() {
 releaseSnapshotBuffer({std::move(buffer_), bufferCapacity_});
}
bool RegionSnapshot::columnHasBlocks(int blockX, int blockZ, int minY, int maxY) const {
 const int clampedMinY = std::max(minY, minY_);
 const int clampedMaxY = std::min(maxY, minY_ + ySpan_);
 if(clampedMinY >= clampedMaxY) {
  return false;
 }
 const std::size_t spanBytes = static_cast<std::size_t>(clampedMaxY - clampedMinY);
 constexpr std::size_t kWordBytes = sizeof(std::uint64_t);
 for(int x = blockX; x < blockX + 16; ++x) {
  for(int z = blockZ; z < blockZ + 16; ++z) {
   const int column = columnIndex(x, z);
   if(column < 0) {
    continue;
   }
   const std::uint8_t* base =
       blocks_ + static_cast<std::size_t>(column * ySpan_ + (clampedMinY - minY_));
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
 }
 return false;
}
RegionSnapshot::ColumnNeighbourhood RegionSnapshot::columnNeighbourhood(int x, int z) const {
 const auto columnPointer = [this](int columnX, int columnZ) -> const std::uint8_t* {
  const int column = columnIndex(columnX, columnZ);
  return column < 0 ? nullptr : blocks_ + static_cast<std::size_t>(column * ySpan_);
 };
 ColumnNeighbourhood columns;
 columns.minY = minY_;
 columns.span = ySpan_;
 columns.self = columnPointer(x, z);
 columns.negX = columnPointer(x - 1, z);
 columns.posX = columnPointer(x + 1, z);
 columns.negZ = columnPointer(x, z - 1);
 columns.posZ = columnPointer(x, z + 1);
 columns.complete = columns.self != nullptr && columns.negX != nullptr && columns.posX != nullptr &&
                    columns.negZ != nullptr && columns.posZ != nullptr;
 return columns;
}
int RegionSnapshot::getBlockId(int x, int y, int z) const {
 const int column = columnIndex(x, z);
 if(column < 0 || !containsY(y)) {
  return 0;
 }
 return static_cast<int>(blocks_[static_cast<std::size_t>(column * ySpan_ + (y - minY_))]);
}
int RegionSnapshot::getBlockMeta(int x, int y, int z) const {
 const int column = columnIndex(x, z);
 if(column < 0 || !containsY(y)) {
  return 0;
 }
 return nibbleAt(meta_, column, y);
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
 const int column = columnIndex(x, z);
 if(y >= Chunk::height || column < 0 || !containsY(y)) {
  const int brightness = 15 - ambientDarkness_;
  return brightness < 0 ? 0 : brightness;
 }
 if(useNeighborLight &&
    block::Block::usesNeighborLightSampling(
        static_cast<int>(blocks_[static_cast<std::size_t>(column * ySpan_ + (y - minY_))]))) {
  int brightness = getRawBrightness(x, y + 1, z, false);
  brightness = std::max(brightness, getRawBrightness(x + 1, y, z, false));
  brightness = std::max(brightness, getRawBrightness(x - 1, y, z, false));
  brightness = std::max(brightness, getRawBrightness(x, y, z + 1, false));
  brightness = std::max(brightness, getRawBrightness(x, y, z - 1, false));
  return brightness;
 }
 int sky = nibbleAt(skyLight_, column, y);
 if(sky > 0) {
  sawSkyLight_ = true;
 }
 const int block = nibbleAt(blockLight_, column, y);
 if(block > (sky -= ambientDarkness_)) {
  sky = block;
 }
 return sky < 0 ? 0 : sky;
}
int RegionSnapshot::getBlockLight(const int x, const int y, const int z) const {
 const int column = columnIndex(x, z);
 if(column < 0 || !containsY(y)) {
  return 0;
 }
 return nibbleAt(blockLight_, column, y);
}
int RegionSnapshot::getSkyLight(const int x, const int y, const int z) const {
 if(y < 0) {
  return 0;
 }
 const int column = columnIndex(x, z);
 if(y >= Chunk::height) {
  return 15;
 }
 if(column < 0 || !containsY(y)) {
  return 15;
 }
 const int sky = nibbleAt(skyLight_, column, y);
 if(sky > 0) {
  sawSkyLight_ = true;
 }
 return sky;
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
 // BLOCKS_OPAQUE is isOpaque() baked per id at registration. Every visible face
 // the mesher emits is decided by six of these, so the virtual call this replaces
 // was one of the densest in the rebuild.
 return block::Block::BLOCKS_OPAQUE[static_cast<std::size_t>(getBlockId(x, y, z))];
}
bool RegionSnapshot::shouldSuffocate(int x, int y, int z) const {
 block::Block* block = block::Block::BLOCKS[static_cast<std::size_t>(getBlockId(x, y, z))];
 if(block == nullptr) {
  return false;
 }
 return block->material.blocksMovement() && block->isFullCube();
}
} // namespace net::minecraft::client::render::chunk
