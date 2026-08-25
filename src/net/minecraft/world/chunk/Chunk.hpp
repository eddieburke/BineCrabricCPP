#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/light/LightType.hpp"
#include "net/minecraft/world/chunk/ChunkNibbleArray.hpp"
namespace net::minecraft {
class World;
class Chunk {
 public:
 static constexpr int width = 16;
 static constexpr int height = 128;
 static constexpr int depth = 16;
 static constexpr std::size_t volume = static_cast<std::size_t>(width * height * depth);
 // Excludes other *bulk* sections only -- the whole-chunk copies (packet load,
 // save serialise, meta resize) whose vectors are read and written wholesale.
 // Per-cell writers do not take it; see noteRenderWrite().
 class RenderWriteGuard {
  public:
  explicit RenderWriteGuard(const Chunk& chunk) : chunk_(chunk) {
   chunk_.lockRenderWrite();
  }
  ~RenderWriteGuard() {
   chunk_.unlockRenderWrite();
  }
  RenderWriteGuard(const RenderWriteGuard&) = delete;
  RenderWriteGuard& operator=(const RenderWriteGuard&) = delete;

  private:
  const Chunk& chunk_;
 };
 static inline bool hasSkyLight{false};
 Chunk() : Chunk(nullptr, 0, 0) {
 }
 Chunk(int x, int z) : Chunk(nullptr, x, z) {
 }
 Chunk(World* world, int x, int z)
     : world(world),
       blocks(volume, 0),
       meta(static_cast<int>(volume)),
       skyLight(static_cast<int>(volume)),
       blockLight(static_cast<int>(volume)),
       x(x),
       z(z) {
  heightmap.fill(0);
 }
 Chunk(World* world, const std::array<std::uint8_t, volume>& sourceBlocks, int x, int z) : Chunk(world, x, z) {
  blocks.assign(sourceBlocks.begin(), sourceBlocks.end());
  refreshSectionBlockCounts();
 }
 Chunk(World* world, const std::vector<std::uint8_t>& sourceBlocks, int x, int z) : Chunk(world, x, z) {
  const std::size_t count = std::min(blocks.size(), sourceBlocks.size());
  std::copy_n(sourceBlocks.begin(), count, blocks.begin());
  refreshSectionBlockCounts();
 }
 ~Chunk();
 Chunk(const Chunk&) = delete;
 Chunk& operator=(const Chunk&) = delete;
  Chunk(Chunk&& other) noexcept
      : world(other.world),
        blocks(std::move(other.blocks)),
        loaded(other.loaded),
        dataReady(other.dataReady),
        meta(std::move(other.meta)),
        skyLight(std::move(other.skyLight)),
        blockLight(std::move(other.blockLight)),
        heightmap(other.heightmap),
        minHeightmapValue(other.minHeightmapValue),
        x(other.x),
        z(other.z),
        blockEntities(std::move(other.blockEntities)),
        entities(std::move(other.entities)),
        terrainPopulated(other.terrainPopulated),
        dirty(other.dirty.load(std::memory_order_relaxed)),
        empty(other.empty),
        lastSaveHadEntities(other.lastSaveHadEntities.load(std::memory_order_relaxed)),
        lastSaveTime(other.lastSaveTime) {
   for(std::size_t section = 0; section < sectionBlockCounts_.size(); ++section) {
    sectionBlockCounts_[section].store(
        other.sectionBlockCounts_[section].load(std::memory_order_relaxed), std::memory_order_relaxed);
    blockLightCounts_[section].store(
        other.blockLightCounts_[section].load(std::memory_order_relaxed), std::memory_order_relaxed);
   }
   other.world = nullptr;
   other.loaded = false;
   other.dataReady = false;
  }
 Chunk& operator=(Chunk&&) = delete;
 [[nodiscard]] bool chunkPosEquals(int chunkX, int chunkZ) const noexcept {
  return chunkX == x && chunkZ == z;
 }
 [[nodiscard]] int getHeight(int localX, int localZ) const {
  const std::uint8_t value =
      std::atomic_ref(const_cast<std::uint8_t&>(heightmap[static_cast<std::size_t>((localZ << 4) | localX)]))
          .load(std::memory_order_relaxed);
  return static_cast<int>(value & 0xFFU);
 }
 void onLoad() {
 }
 void populateHeightMapOnly() {
  const RenderWriteGuard guard(*this);
  populateHeightMapOnlyUnlocked();
 }
 void populateHeightMapOnlyUnlocked() {
  int minHeight = 127;
  for(int localX = 0; localX < 16; ++localX) {
   for(int localZ = 0; localZ < 16; ++localZ) {
    int topY = findTopBlock(localX, localZ);
    std::atomic_ref<std::uint8_t>(heightmap[static_cast<std::size_t>((localZ << 4) | localX)])
        .store(static_cast<std::uint8_t>(topY), std::memory_order_relaxed);
    if(topY < minHeight) {
     minHeight = topY;
    }
   }
  }
  minHeightmapValue = minHeight;
  refreshSectionBlockCountsUnlocked();
  dirty = true;
 }
 void populateHeightMap(bool fixCrossChunkGaps = true);
 void recalculateHeightMap() {
  populateHeightMap();
 }
 void populateBlockLight();
 [[nodiscard]] bool sectionHasBlocks(int sectionY) const noexcept {
  return sectionY >= 0 && sectionY < static_cast<int>(sectionBlockCounts_.size()) &&
         sectionBlockCounts_[static_cast<std::size_t>(sectionY)].load(std::memory_order_relaxed) != 0;
 }
 [[nodiscard]] int nonEmptySectionCount() const noexcept {
  int count = 0;
  for(const auto& section : sectionBlockCounts_) {
   count += section.load(std::memory_order_relaxed) != 0 ? 1 : 0;
  }
  return count;
 }
 [[nodiscard]] std::uint8_t blockLightSectionMask() const noexcept {
  std::uint8_t mask = 0;
  for(std::size_t section = 0; section < blockLightCounts_.size(); ++section) {
   if(blockLightCounts_[section].load(std::memory_order_relaxed) != 0) {
    mask = static_cast<std::uint8_t>(mask | (1U << section));
   }
  }
  return mask;
 }
 void refreshBlockLightCounts() {
  const RenderWriteGuard guard(*this);
  refreshBlockLightCountsUnlocked();
 }
 void relightSkylightGaps();
 void attachToWorld(World* worldIn) noexcept {
  world = worldIn;
  for(auto& entry : blockEntities) {
   if(entry.second != nullptr) {
    entry.second->world = worldIn;
   }
  }
  for(auto& slice : entities) {
   for(Entity* entity : slice) {
    if(entity != nullptr) {
     entity->world = worldIn;
    }
   }
  }
 }
 [[nodiscard]] int getBlockId(int localX, int yPos, int localZ) const {
  const std::uint8_t value =
      std::atomic_ref(const_cast<std::uint8_t&>(blocks[index(localX, yPos, localZ)]))
          .load(std::memory_order_relaxed);
  return static_cast<int>(value & 0xFFU);
 }
 bool setBlock(int localX, int yPos, int localZ, int rawId, int metadataValue);
 bool setBlock(int localX, int yPos, int localZ, int rawId);
 [[nodiscard]] int getBlockMeta(int localX, int yPos, int localZ) const {
  return meta.get(localX, yPos, localZ);
 }
 void setBlockMeta(int localX, int yPos, int localZ, int metadataValue);
 [[nodiscard]] int getLight(LightType lightType, int localX, int yPos, int localZ) const {
  return lightType == LightType::Sky ? skyLight.get(localX, yPos, localZ) : blockLight.get(localX, yPos, localZ);
 }
 void setLight(LightType lightType, int localX, int yPos, int localZ, int value) {
  // ChunkNibbleArray::set is a byte-wise CAS, so two light workers sharing a
  // byte cannot drop each other's nibble, and the mesh capture reads without
  // locking. This is the innermost write of the light relaxation -- it took and
  // released the chunk's write flag once per cell, and every one of those
  // acquisitions could land behind a whole-chunk mesh capture.
  if(lightType == LightType::Sky) {
   skyLight.set(localX, yPos, localZ, value);
  } else {
   const int previous = blockLight.get(localX, yPos, localZ);
   blockLight.set(localX, yPos, localZ, value);
   updateBlockLightCount(yPos, previous, value);
  }
  dirty = true;
 }
 [[nodiscard]] int getLight(int localX, int yPos, int localZ, int ambientDarkness) const {
  int sky = skyLight.get(localX, yPos, localZ);
  if(sky > 0) {
    hasSkyLight = true;
  }
  const int block = blockLight.get(localX, yPos, localZ);
  if(block > (sky -= ambientDarkness)) {
   sky = block;
  }
  return sky < 0 ? 0 : sky;
 }
  void addEntity(Entity* entity) {
   if(entity == nullptr) {
    return;
   }
   const std::lock_guard lock(*entityMutex_);
   lastSaveHadEntities = true;
   int slice = floor_int(entity->y / 16.0);
   if(slice < 0) {
    slice = 0;
   }
   if(slice >= static_cast<int>(entities.size())) {
    slice = static_cast<int>(entities.size()) - 1;
   }
   entity->isPersistent = true;
   entity->chunkX = x;
   entity->chunkSlice = slice;
   entity->chunkZ = z;
   auto& entitySlice = entities[static_cast<std::size_t>(slice)];
   if(std::find(entitySlice.begin(), entitySlice.end(), entity) == entitySlice.end()) {
    entitySlice.push_back(entity);
   }
  }
  void removeEntity(Entity* entity) {
   if(entity != nullptr) {
    removeEntity(entity, entity->chunkSlice);
   }
  }
  void removeEntity(Entity* entity, int chunkSlice) {
   const std::lock_guard lock(*entityMutex_);
   if(chunkSlice < 0) {
    chunkSlice = 0;
   }
   if(chunkSlice >= static_cast<int>(entities.size())) {
    chunkSlice = static_cast<int>(entities.size()) - 1;
   }
   auto& slice = entities[static_cast<std::size_t>(chunkSlice)];
   slice.erase(std::remove(slice.begin(), slice.end(), entity), slice.end());
  }
 [[nodiscard]] bool isAboveMaxHeight(int localX, int yPos, int localZ) const {
  return yPos >= getHeight(localX, localZ);
 }
 [[nodiscard]] block::entity::BlockEntity* getBlockEntity(int localX, int yPos, int localZ);
 void addBlockEntity(std::unique_ptr<block::entity::BlockEntity> blockEntity) {
  if(!blockEntity) {
   return;
  }
  const int localX = blockEntity->x - x * 16;
  const int yPos = blockEntity->y;
  const int localZ = blockEntity->z - z * 16;
  setBlockEntity(localX, yPos, localZ, std::move(blockEntity));
 }
 void setBlockEntity(int localX, int yPos, int localZ, std::unique_ptr<block::entity::BlockEntity> blockEntity);
 void removeBlockEntityAt(int localX, int yPos, int localZ);
  void load();
  void unload();
  // Render/lighting leases. A worker may only dereference a Chunk it holds a
  // lease on; the cache tombstones evicted chunks into a graveyard and frees
  // them only once the lease count reaches zero, so the object stays valid for
  // any worker that acquired a lease before eviction. Acquisition races with
  // eviction by design: an increment that lands after markEvicted() backs out
  // (the worker then treats the chunk as absent), and every lease taken before
  // eviction is counted, so the free-at-zero sweep cannot race an increment.
  [[nodiscard]] bool tryAcquireRenderPin() noexcept;
  void releaseRenderPin() noexcept;
  // Marks the chunk evicted. Once set, new leases fail; existing leases keep
  // the object alive until they drain. Caller must own the chunk (main thread,
  // chunk removed from all maps) before or immediately after.
  void markEvicted() noexcept {
   evicted_.store(true, std::memory_order_release);
  }
  [[nodiscard]] bool isEvicted() const noexcept {
   return evicted_.load(std::memory_order_acquire);
  }
  [[nodiscard]] std::uint32_t renderPinCount() const noexcept {
   return renderPins_.load(std::memory_order_acquire);
  }
  // Blocks (bounded) until every lease on this chunk is released. Teardown
  // only: the cache uses it before destroying graveyard entries.
  static void waitForPinDrain(const Chunk& chunk, std::chrono::milliseconds timeout);
  void markDirty() {
   dirty = true;
  }
  void collectOtherEntities(Entity* except, const Box& box, std::vector<Entity*>& result) {
   const std::lock_guard lock(*entityMutex_);
   int minSlice = floor_int((box.minY - 2.0) / 16.0);
  int maxSlice = floor_int((box.maxY + 2.0) / 16.0);
  minSlice = std::max(minSlice, 0);
  maxSlice = std::min(maxSlice, static_cast<int>(entities.size()) - 1);
  for(int slice = minSlice; slice <= maxSlice; ++slice) {
   for(Entity* entity : entities[static_cast<std::size_t>(slice)]) {
    if(entity != nullptr && entity != except && entity->boundingBox.intersects(box)) {
     result.push_back(entity);
    }
   }
  }
 }
  template <typename T>
  void collectEntitiesByClass(const Box& box, std::vector<T*>& result) {
   const std::lock_guard lock(*entityMutex_);
   int minSlice = floor_int((box.minY - 2.0) / 16.0);
  int maxSlice = floor_int((box.maxY + 2.0) / 16.0);
  minSlice = std::max(minSlice, 0);
  maxSlice = std::min(maxSlice, static_cast<int>(entities.size()) - 1);
  for(int slice = minSlice; slice <= maxSlice; ++slice) {
   for(Entity* entity : entities[static_cast<std::size_t>(slice)]) {
    if(entity == nullptr || !entity->boundingBox.intersects(box)) {
     continue;
    }
    if(auto* typed = dynamic_cast<T*>(entity)) {
     result.push_back(typed);
    }
   }
  }
 }
 [[nodiscard]] bool shouldSave(bool saveEntities) const {
  if(empty) {
   return false;
  }
  if(!terrainPopulated && !dirty) {
   return false;
  }
  return dirty || (saveEntities && lastSaveHadEntities);
 }
 int loadFromPacket(const std::vector<std::uint8_t>& bytes,
                    int minX,
                    int minY,
                    int minZ,
                    int maxX,
                    int maxY,
                    int maxZ,
                    int offset) {
  const RenderWriteGuard guard(*this);
   for(int localX = minX; localX < maxX; ++localX) {
    for(int localZ = minZ; localZ < maxZ; ++localZ) {
     const std::size_t dest = index(localX, minY, localZ);
     const int count = maxY - minY;
     std::copy_n(bytes.begin() + offset, count, blocks.begin() + static_cast<std::ptrdiff_t>(dest));
     offset += count;
    }
   }
  populateHeightMapOnlyUnlocked();
   for(int localX = minX; localX < maxX; ++localX) {
    for(int localZ = minZ; localZ < maxZ; ++localZ) {
     const std::size_t dest = index(localX, minY, localZ) >> 1U;
     const int count = (maxY - minY) / 2;
     std::copy_n(bytes.begin() + offset, count, meta.bytes.begin() + static_cast<std::ptrdiff_t>(dest));
     offset += count;
    }
   }
   for(int localX = minX; localX < maxX; ++localX) {
    for(int localZ = minZ; localZ < maxZ; ++localZ) {
     const std::size_t dest = index(localX, minY, localZ) >> 1U;
     const int count = (maxY - minY) / 2;
     std::copy_n(bytes.begin() + offset, count, blockLight.bytes.begin() + static_cast<std::ptrdiff_t>(dest));
     offset += count;
    }
   }
  refreshBlockLightCountsUnlocked();
   for(int localX = minX; localX < maxX; ++localX) {
    for(int localZ = minZ; localZ < maxZ; ++localZ) {
     const std::size_t dest = index(localX, minY, localZ) >> 1U;
     const int count = (maxY - minY) / 2;
     std::copy_n(bytes.begin() + offset, count, skyLight.bytes.begin() + static_cast<std::ptrdiff_t>(dest));
     offset += count;
    }
   }
  dirty = true;
  return offset;
 }
 [[nodiscard]] int toPacket(std::vector<std::uint8_t>& bytes,
                            int minX,
                            int minY,
                            int minZ,
                            int maxX,
                            int maxY,
                            int maxZ,
                            int offset) const {
  const RenderWriteGuard guard(*this);
  const int sizeX = maxX - minX;
  const int sizeY = maxY - minY;
  const int sizeZ = maxZ - minZ;
  const std::size_t needed =
      static_cast<std::size_t>(offset + sizeX * sizeY * sizeZ + (sizeX * sizeY * sizeZ / 2) * 3);
  if(bytes.size() < needed) {
   bytes.resize(needed);
  }
  if(sizeX * sizeY * sizeZ == static_cast<int>(blocks.size())) {
   std::copy(blocks.begin(), blocks.end(), bytes.begin() + offset);
   offset += static_cast<int>(blocks.size());
   std::copy(meta.bytes.begin(), meta.bytes.end(), bytes.begin() + offset);
   offset += static_cast<int>(meta.bytes.size());
   std::copy(blockLight.bytes.begin(), blockLight.bytes.end(), bytes.begin() + offset);
   offset += static_cast<int>(blockLight.bytes.size());
   std::copy(skyLight.bytes.begin(), skyLight.bytes.end(), bytes.begin() + offset);
   offset += static_cast<int>(skyLight.bytes.size());
   return offset;
  }
  for(int localX = minX; localX < maxX; ++localX) {
   for(int localZ = minZ; localZ < maxZ; ++localZ) {
    const std::size_t source = index(localX, minY, localZ);
    std::copy_n(blocks.begin() + static_cast<std::ptrdiff_t>(source), sizeY, bytes.begin() + offset);
    offset += sizeY;
   }
  }
  for(int localX = minX; localX < maxX; ++localX) {
   for(int localZ = minZ; localZ < maxZ; ++localZ) {
    const std::size_t source = index(localX, minY, localZ) >> 1U;
    const int count = sizeY / 2;
    std::copy_n(meta.bytes.begin() + static_cast<std::ptrdiff_t>(source), count, bytes.begin() + offset);
    offset += count;
   }
  }
  for(int localX = minX; localX < maxX; ++localX) {
   for(int localZ = minZ; localZ < maxZ; ++localZ) {
    const std::size_t source = index(localX, minY, localZ) >> 1U;
    const int count = sizeY / 2;
    std::copy_n(
        blockLight.bytes.begin() + static_cast<std::ptrdiff_t>(source), count, bytes.begin() + offset);
    offset += count;
   }
  }
  for(int localX = minX; localX < maxX; ++localX) {
   for(int localZ = minZ; localZ < maxZ; ++localZ) {
    const std::size_t source = index(localX, minY, localZ) >> 1U;
    const int count = sizeY / 2;
    std::copy_n(
        skyLight.bytes.begin() + static_cast<std::ptrdiff_t>(source), count, bytes.begin() + offset);
    offset += count;
   }
  }
  return offset;
 }
 [[nodiscard]] JavaRandom getSlimeRandom(long long scrambler) const {
  const long long seed =
      ((x * x * 4987142LL) + (x * 5947611LL) + (z * z) * 4392871LL + (z * 389711LL)) ^ scrambler;
  return JavaRandom(static_cast<std::uint64_t>(seed));
 }
 [[nodiscard]] bool isEmpty() const {
  return empty;
 }
  void fill() {
   const RenderWriteGuard guard(*this);
   for(std::size_t i = 0; i < blocks.size(); ++i) {
    const std::uint8_t blockId = blocks[i];
    blocks[i] = Block::BLOCKS[static_cast<std::size_t>(blockId)] == nullptr ? 0 : blockId;
   }
   refreshSectionBlockCountsUnlocked();
  }
 World* world = nullptr;
 std::vector<std::uint8_t> blocks;
 bool loaded = false;
 bool dataReady = false;
 ChunkNibbleArray meta;
 ChunkNibbleArray skyLight;
 ChunkNibbleArray blockLight;
 std::array<std::uint8_t, 256> heightmap{};
 int minHeightmapValue = 0;
 const int x = 0;
 const int z = 0;
  std::unordered_map<Vec3i, std::unique_ptr<block::entity::BlockEntity>, Vec3iHash> blockEntities{};
  std::array<std::vector<Entity*>, 8> entities{};
  bool terrainPopulated = false;
  std::atomic<bool> dirty{false};
  bool empty = false;
  std::atomic<bool> lastSaveHadEntities{false};
  long long lastSaveTime = 0;
  std::array<std::atomic<std::uint16_t>, 8> sectionBlockCounts_{};
  std::array<std::atomic<std::uint16_t>, 8> blockLightCounts_{};
  // Seqlock version for bulk sections only. Even means none is in flight; a bulk
  // writer makes it odd for the length of its section and leaves it changed, so a
  // reader that copied across one can tell.
  std::unique_ptr<std::atomic<std::uint32_t>> renderSeq_ =
      std::make_unique<std::atomic<std::uint32_t>>(0);
  // Guards the entity/block-entity containers. The main thread mutates them
  // (entity ticks, block-entity placement, load/unload); the save drain
  // serializes them into a snapshot on an IO worker. recursive so a block
  // entity created from inside getBlockEntity()'s onPlaced callback can
  // re-enter the chunk while the outer read holds the lock.
  std::unique_ptr<std::recursive_mutex> entityMutex_ =
      std::make_unique<std::recursive_mutex>();
  std::atomic<std::uint32_t> renderPins_{0};
  std::atomic<bool> evicted_{false};

 public:
 void lockRenderWrite() const noexcept;
 void unlockRenderWrite() const noexcept;
 // Optimistic-read stamp. An odd value means a bulk section holds the chunk;
 // the caller retries rather than blocking. Per-cell writers are not part of
 // this protocol at all -- every one of their stores is already byte-atomic
 // (atomic_ref on blocks/heightmap, a CAS on the nibble arrays), so a reader
 // that races one gets a value that is stale, never torn, and the write is
 // followed by a dirty-region publish that remeshes it anyway.
 [[nodiscard]] std::uint32_t renderVersion() const noexcept {
  return renderSeq_->load(std::memory_order_acquire);
 }
 [[nodiscard]] bool renderReadValid(std::uint32_t stamp) const noexcept {
  return (stamp & 1U) == 0 && renderSeq_->load(std::memory_order_acquire) == stamp;
 }

 private:
 [[nodiscard]] static constexpr std::size_t index(int localX, int yPos, int localZ) {
  return static_cast<std::size_t>((localX << 11) | (localZ << 7) | yPos);
 }
 [[nodiscard]] int findTopBlock(int localX, int localZ) const {
  const int base = (localX << 11) | (localZ << 7);
  int topY = 127;
  while(topY > 0 && Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(
                        std::atomic_ref(const_cast<std::uint8_t&>(blocks[static_cast<std::size_t>(base + topY - 1)]))
                            .load(std::memory_order_relaxed) &
                        0xFFU)] == 0) {
   --topY;
  }
  return topY;
 }
 void recalculateHeightColumn(int localX, int localZ) {
  const RenderWriteGuard guard(*this);
  const int topY = findTopBlock(localX, localZ);
  std::atomic_ref<std::uint8_t>(heightmap[static_cast<std::size_t>((localZ << 4) | localX)])
      .store(static_cast<std::uint8_t>(topY), std::memory_order_relaxed);
  minHeightmapValue = topY;
 }
 void updateHeightMap(int localX, int yPos, int localZ);
 void updateSectionBlockCount(int yPos, int previousId, int rawId) noexcept;
 void updateBlockLightCount(int yPos, int previous, int value) noexcept;
 void refreshSectionBlockCounts();
 void refreshSectionBlockCountsUnlocked() noexcept;
 void refreshBlockLightCountsUnlocked() noexcept;
 void lightGaps(int localX, int localZ);
 void lightGap(int blockX, int blockZ, int yPos);
};
} // namespace net::minecraft
