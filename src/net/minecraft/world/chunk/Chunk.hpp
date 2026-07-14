#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/LightType.hpp"
#include "net/minecraft/world/chunk/BlockSource.hpp"
#include "net/minecraft/world/chunk/ChunkNibbleArray.hpp"
namespace net::minecraft {
class World;
class Chunk {
public:
  static constexpr int width = 16;
  static constexpr int height = 128;
  static constexpr int depth = 16;
  static constexpr std::size_t volume = static_cast<std::size_t>(width * height * depth);
  static inline bool hasSkyLight = false;
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
  }
  Chunk(World* world, const std::vector<std::uint8_t>& sourceBlocks, int x, int z) : Chunk(world, x, z) {
    const std::size_t count = std::min(blocks.size(), sourceBlocks.size());
    std::copy_n(sourceBlocks.begin(), count, blocks.begin());
  }
  ~Chunk();
  Chunk(const Chunk&) = delete;
  Chunk& operator=(const Chunk&) = delete;
  Chunk(Chunk&& other) noexcept
      : world(other.world),
        blocks(std::move(other.blocks)),
        loaded(other.loaded),
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
        dirty(other.dirty),
        empty(other.empty),
        lastSaveHadEntities(other.lastSaveHadEntities),
        lastSaveTime(other.lastSaveTime) {
    other.world = nullptr;
    other.loaded = false;
  }
  Chunk& operator=(Chunk&&) = delete;
  [[nodiscard]] bool chunkPosEquals(int chunkX, int chunkZ) const noexcept {
    return chunkX == x && chunkZ == z;
  }
  [[nodiscard]] int getHeight(int localX, int localZ) const {
    return static_cast<int>(heightmap[static_cast<std::size_t>((localZ << 4) | localX)] & 0xFFU);
  }
  void onLoad() {
  }
  void populateHeightMapOnly() {
    int minHeight = 127;
    for(int localX = 0; localX < 16; ++localX) {
      for(int localZ = 0; localZ < 16; ++localZ) {
        int topY = findTopBlock(localX, localZ);
        heightmap[static_cast<std::size_t>((localZ << 4) | localX)] = static_cast<std::uint8_t>(topY);
        if(topY < minHeight) {
          minHeight = topY;
        }
      }
    }
    minHeightmapValue = minHeight;
    dirty = true;
  }
  void populateHeightMap(bool fixCrossChunkGaps = true);
  void recalculateHeightMap() {
    populateHeightMap();
  }
  void populateBlockLight() {
  }
  // Re-queue cross-chunk skylight gap fixes once this chunk and neighbors are loaded.
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
    return static_cast<int>(blocks[index(localX, yPos, localZ)] & 0xFFU);
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
    if(lightType == LightType::Sky) {
      skyLight.set(localX, yPos, localZ, value);
    } else {
      blockLight.set(localX, yPos, localZ, value);
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
    entities[static_cast<std::size_t>(slice)].push_back(entity);
  }
  void removeEntity(Entity* entity) {
    if(entity != nullptr) {
      removeEntity(entity, entity->chunkSlice);
    }
  }
  void removeEntity(Entity* entity, int chunkSlice) {
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
  [[nodiscard]] bool tryAcquireRenderPin() noexcept {
    if(renderEvicting_.load(std::memory_order_acquire)) {
      return false;
    }
    renderPinCount_.fetch_add(1, std::memory_order_acquire);
    if(!renderEvicting_.load(std::memory_order_acquire)) {
      return true;
    }
    releaseRenderPin();
    return false;
  }
  void releaseRenderPin() noexcept {
    renderPinCount_.fetch_sub(1, std::memory_order_release);
  }
  [[nodiscard]] bool beginRenderEviction() noexcept {
    renderEvicting_.store(true, std::memory_order_release);
    return renderPinCount_.load(std::memory_order_acquire) == 0;
  }
  void cancelRenderEviction() noexcept {
    renderEvicting_.store(false, std::memory_order_release);
  }
  void markDirty() {
    dirty = true;
  }
  void collectOtherEntities(Entity* except, const Box& box, std::vector<Entity*>& result) {
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
    // Prefetch-only chunks: terrain shell without decoration and no player edits.
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
    for(int localX = minX; localX < maxX; ++localX) {
      for(int localZ = minZ; localZ < maxZ; ++localZ) {
        const std::size_t dest = index(localX, minY, localZ);
        const int count = maxY - minY;
        std::copy_n(bytes.begin() + offset, count, blocks.begin() + static_cast<std::ptrdiff_t>(dest));
        offset += count;
      }
    }
    populateHeightMapOnly();
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
        std::copy_n(
            bytes.begin() + offset, count, blockLight.bytes.begin() + static_cast<std::ptrdiff_t>(dest));
        offset += count;
      }
    }
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
    std::vector<std::uint8_t> copy(blocks.begin(), blocks.end());
    BlockSource::fill(copy);
    std::copy(copy.begin(), copy.end(), blocks.begin());
  }
  World* world = nullptr;
  std::vector<std::uint8_t> blocks;
  bool loaded = false;
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
  bool dirty = false;
  bool empty = false;
  bool lastSaveHadEntities = false;
  long long lastSaveTime = 0;

private:
  std::atomic<int> renderPinCount_{0};
  std::atomic<bool> renderEvicting_{false};
  [[nodiscard]] static constexpr std::size_t index(int localX, int yPos, int localZ) {
    return static_cast<std::size_t>((localX << 11) | (localZ << 7) | yPos);
  }
  [[nodiscard]] int findTopBlock(int localX, int localZ) const {
    const int base = (localX << 11) | (localZ << 7);
    int topY = 127;
    while(topY > 0 && Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(
                          blocks[static_cast<std::size_t>(base + topY - 1)] & 0xFFU)] == 0) {
      --topY;
    }
    return topY;
  }
  void recalculateHeightColumn(int localX, int localZ) {
    const int topY = findTopBlock(localX, localZ);
    heightmap[static_cast<std::size_t>((localZ << 4) | localX)] = static_cast<std::uint8_t>(topY);
    minHeightmapValue = topY;
  }
  void updateHeightMap(int localX, int yPos, int localZ);
  void lightGaps(int localX, int localZ);
  void lightGap(int blockX, int blockZ, int yPos);
};
} // namespace net::minecraft
