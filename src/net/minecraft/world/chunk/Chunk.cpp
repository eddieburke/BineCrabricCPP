#include "net/minecraft/world/chunk/Chunk.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/BlockWithEntity.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft {
namespace {
// Teardown-only drain coordination: the cache waits (bounded) for leases on
// graveyard chunks to drop to zero before destroying them. Notified by the
// worker that releases the last lease.
std::mutex gPinMutex;
std::condition_variable gPinCv;
} // namespace
bool Chunk::tryAcquireRenderPin() noexcept {
 renderPins_.fetch_add(1, std::memory_order_acq_rel);
 if(evicted_.load(std::memory_order_acquire)) {
  releaseRenderPin();
  return false;
 }
 return true;
}
void Chunk::releaseRenderPin() noexcept {
 const std::uint32_t remaining = renderPins_.fetch_sub(1, std::memory_order_acq_rel);
 if(remaining == 1) {
  gPinCv.notify_all();
 }
}
void Chunk::waitForPinDrain(const Chunk& chunk, std::chrono::milliseconds timeout) {
 std::unique_lock lock(gPinMutex);
 gPinCv.wait_for(lock, timeout, [&chunk] { return chunk.renderPinCount() == 0; });
}
void Chunk::lockRenderWrite() const noexcept {
 unsigned spins = 0;
 for(;;) {
  std::uint32_t seq = renderSeq_->load(std::memory_order_relaxed);
  if((seq & 1U) == 0 &&
     renderSeq_->compare_exchange_weak(seq, seq + 1, std::memory_order_acquire, std::memory_order_relaxed)) {
   return;
  }
  if(++spins >= 64) {
   spins = 0;
   std::this_thread::yield();
  }
 }
}
void Chunk::unlockRenderWrite() const noexcept {
 // Leaves the version even again and, crucially, *changed* -- a reader that
 // copied across this section fails its stamp check and retries.
 renderSeq_->fetch_add(1, std::memory_order_release);
}
Chunk::~Chunk() {
 if(loaded) {
  unload();
 }
}
bool Chunk::setBlock(int localX, int yPos, int localZ, int rawId, int metadataValue) {
 const int topHeight = static_cast<int>(
     std::atomic_ref(const_cast<std::uint8_t&>(heightmap[static_cast<std::size_t>((localZ << 4) | localX)]))
         .load(std::memory_order_relaxed) &
     0xFFU);
 const int previousId = getBlockId(localX, yPos, localZ);
 const int previousMeta = meta.get(localX, yPos, localZ);
 if(previousId == rawId && previousMeta == metadataValue) {
  return false;
 }
 const int blockX = this->x * 16 + localX;
 const int blockZ = this->z * 16 + localZ;
 // Both stores are byte-atomic and world->setBlockDirty below forces the remesh,
 // so a mesh capture racing this sees one stale value, never a torn one, and is
 // corrected on the next frame. Not worth making the capture wait for.
 std::atomic_ref<std::uint8_t>(blocks[index(localX, yPos, localZ)])
     .store(static_cast<std::uint8_t>(rawId & 0xFF), std::memory_order_relaxed);
 updateSectionBlockCount(yPos, previousId, rawId);
 meta.set(localX, yPos, localZ, metadataValue);
 if(previousId != 0 && world != nullptr && !world->isRemote()) {
  Block* previousBlock = Block::BLOCKS[static_cast<std::size_t>(previousId)];
  if(previousBlock != nullptr) {
   previousBlock->onBreak(world, blockX, yPos, blockZ);
  }
 }
 if(world != nullptr) {
  const bool hasSkyLight = world->dimension == nullptr || !world->dimension->hasCeiling;
  if(hasSkyLight) {
   if(Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(rawId & 0xFF)] != 0) {
    if(yPos >= topHeight) {
     updateHeightMap(localX, yPos + 1, localZ);
    }
   } else if(yPos == topHeight - 1) {
    updateHeightMap(localX, yPos, localZ);
   }
   world->queueLightUpdate(LightType::Sky, blockX, yPos, blockZ, blockX, yPos, blockZ);
  }
  world->queueLightUpdate(LightType::Block, blockX, yPos, blockZ, blockX, yPos, blockZ);
  lightGaps(localX, localZ);
 }
 if(rawId != 0 && world != nullptr) {
  Block* placedBlock = Block::BLOCKS[static_cast<std::size_t>(rawId)];
  if(placedBlock != nullptr) {
   placedBlock->onPlaced(world, blockX, yPos, blockZ);
  }
  mod::runtime::WorldRequiredMods::notePlaced(world, rawId);
 }
 if(world != nullptr) {
  world->setBlockDirty(blockX, yPos, blockZ);
 }
 dirty = true;
 return true;
}
bool Chunk::setBlock(int localX, int yPos, int localZ, int rawId) {
 const int topHeight = static_cast<int>(
     std::atomic_ref(const_cast<std::uint8_t&>(heightmap[static_cast<std::size_t>((localZ << 4) | localX)]))
         .load(std::memory_order_relaxed) &
     0xFFU);
 const int previousId = getBlockId(localX, yPos, localZ);
 if(previousId == rawId) {
  return false;
 }
 const int blockX = this->x * 16 + localX;
 const int blockZ = this->z * 16 + localZ;
 std::atomic_ref<std::uint8_t>(blocks[index(localX, yPos, localZ)])
     .store(static_cast<std::uint8_t>(rawId & 0xFF), std::memory_order_relaxed);
 updateSectionBlockCount(yPos, previousId, rawId);
 meta.set(localX, yPos, localZ, 0);
 if(previousId != 0 && world != nullptr) {
  Block* previousBlock = Block::BLOCKS[static_cast<std::size_t>(previousId)];
  if(previousBlock != nullptr) {
   previousBlock->onBreak(world, blockX, yPos, blockZ);
  }
 }
 if(world != nullptr) {
  if(Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(rawId & 0xFF)] != 0) {
   if(yPos >= topHeight) {
    updateHeightMap(localX, yPos + 1, localZ);
   }
  } else if(yPos == topHeight - 1) {
   updateHeightMap(localX, yPos, localZ);
  }
  world->queueLightUpdate(LightType::Sky, blockX, yPos, blockZ, blockX, yPos, blockZ);
  world->queueLightUpdate(LightType::Block, blockX, yPos, blockZ, blockX, yPos, blockZ);
  lightGaps(localX, localZ);
  if(rawId != 0 && !world->isRemote()) {
   Block* placedBlock = Block::BLOCKS[static_cast<std::size_t>(rawId)];
   if(placedBlock != nullptr) {
    placedBlock->onPlaced(world, blockX, yPos, blockZ);
   }
  }
  if(rawId != 0) {
   mod::runtime::WorldRequiredMods::notePlaced(world, rawId);
  }
  world->setBlockDirty(blockX, yPos, blockZ);
 }
 dirty = true;
 return true;
}
void Chunk::setBlockMeta(int localX, int yPos, int localZ, int metadataValue) {
 const int previousMeta = meta.get(localX, yPos, localZ);
 if(previousMeta == metadataValue) {
  return;
 }
 meta.set(localX, yPos, localZ, metadataValue);
 dirty = true;
 if(world != nullptr) {
  world->setBlockDirty(x * 16 + localX, yPos, z * 16 + localZ);
 }
}
void Chunk::updateSectionBlockCount(int yPos, int previousId, int rawId) noexcept {
 const bool previousPresent = (previousId & 0xFF) != 0;
 const bool currentPresent = (rawId & 0xFF) != 0;
 if(previousPresent == currentPresent || yPos < 0 || yPos >= height) {
  return;
 }
 auto& count = sectionBlockCounts_[static_cast<std::size_t>(yPos >> 4)];
 if(currentPresent) {
  count.fetch_add(1, std::memory_order_relaxed);
 } else {
  count.fetch_sub(1, std::memory_order_relaxed);
 }
}
void Chunk::refreshSectionBlockCounts() {
 const RenderWriteGuard guard(*this);
 refreshSectionBlockCountsUnlocked();
}
void Chunk::refreshSectionBlockCountsUnlocked() noexcept {
 std::array<std::uint16_t, 8> counts{};
 for(std::size_t i = 0; i < blocks.size(); ++i) {
  const std::uint8_t blockId =
      std::atomic_ref(const_cast<std::uint8_t&>(blocks[i])).load(std::memory_order_relaxed);
  if(blockId != 0) {
   ++counts[(i & 127U) >> 4U];
  }
 }
 for(std::size_t section = 0; section < counts.size(); ++section) {
  sectionBlockCounts_[section].store(counts[section], std::memory_order_relaxed);
 }
}
void Chunk::updateBlockLightCount(int yPos, int previous, int value) noexcept {
 const bool previousPresent = (previous & 15) != 0;
 const bool currentPresent = (value & 15) != 0;
 if(previousPresent == currentPresent || yPos < 0 || yPos >= height) {
  return;
 }
 auto& count = blockLightCounts_[static_cast<std::size_t>(yPos >> 4)];
 if(currentPresent) {
  count.fetch_add(1, std::memory_order_relaxed);
 } else {
  count.fetch_sub(1, std::memory_order_relaxed);
 }
}
void Chunk::refreshBlockLightCountsUnlocked() noexcept {
 std::array<std::uint16_t, 8> counts{};
 for(int localX = 0; localX < width; ++localX) {
  for(int localZ = 0; localZ < depth; ++localZ) {
   for(int yPos = 0; yPos < height; ++yPos) {
    if(blockLight.get(localX, yPos, localZ) != 0) {
     ++counts[static_cast<std::size_t>(yPos >> 4)];
    }
   }
  }
 }
 for(std::size_t section = 0; section < counts.size(); ++section) {
  blockLightCounts_[section].store(counts[section], std::memory_order_relaxed);
 }
}
void Chunk::relightSkylightGaps() {
 if(world == nullptr || world->dimension == nullptr || world->dimension->hasCeiling) {
  return;
 }
 for(int localX = 0; localX < 16; ++localX) {
  for(int localZ = 0; localZ < 16; ++localZ) {
   lightGaps(localX, localZ);
  }
 }
}
void Chunk::populateBlockLight() {
 if(world == nullptr || world->isRemote()) {
  return;
 }
 if(blockLightSectionMask() != 0) {
  return;
 }
 const int blockX = this->x * 16;
 const int blockZ = this->z * 16;
 std::uint8_t activeSections = 0;
 for(int sectionY = 0; sectionY < 8; ++sectionY) {
  if(sectionHasBlocks(sectionY)) {
   activeSections = static_cast<std::uint8_t>(activeSections | (1U << sectionY));
  }
 }
 constexpr int neighborX[4] = {-1, 16, 8, 8};
 constexpr int neighborZ[4] = {8, 8, -1, 16};
 for(int direction = 0; direction < 4; ++direction) {
  const Chunk* neighbor = world->getChunkIfLoaded(blockX + neighborX[direction], blockZ + neighborZ[direction]);
  if(neighbor != nullptr && neighbor != this) {
   activeSections = static_cast<std::uint8_t>(activeSections | neighbor->blockLightSectionMask());
  }
 }
 for(int sectionY = 0; sectionY < 8; ++sectionY) {
  if((activeSections & static_cast<std::uint8_t>(1U << sectionY)) == 0) {
   continue;
  }
  const int minY = sectionY * 16;
  world->queueLightUpdate(LightType::Block, blockX, minY, blockZ, blockX + 15, minY + 15, blockZ + 15);
 }
 if(activeSections != 0) {
  dirty = true;
 }
}
void Chunk::populateHeightMap(bool fixCrossChunkGaps) {
 assert(registry::Registry::isBootstrapped() && "Chunk: call Registry::bootstrap() before populateHeightMap");
 int minHeight = 127;
 const bool skipSkyLight = world != nullptr && world->dimension != nullptr && world->dimension->hasCeiling;
 {
  const RenderWriteGuard guard(*this);
  for(int localX = 0; localX < 16; ++localX) {
   for(int localZ = 0; localZ < 16; ++localZ) {
   const int topY = findTopBlock(localX, localZ);
   std::atomic_ref<std::uint8_t>(heightmap[static_cast<std::size_t>((localZ << 4) | localX)])
       .store(static_cast<std::uint8_t>(topY), std::memory_order_relaxed);
   if(topY < minHeight) {
    minHeight = topY;
   }
    if(skipSkyLight) {
     continue;
    }
    int light = 15;
    const int base = (localX << 11) | (localZ << 7);
    int yPos = 127;
    do {
     light -= Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(
         std::atomic_ref(const_cast<std::uint8_t&>(blocks[static_cast<std::size_t>(base + yPos)]))
             .load(std::memory_order_relaxed) &
         0xFFU)];
     if(light > 0) {
      skyLight.set(localX, yPos, localZ, light);
     }
    } while(--yPos > 0 && light > 0);
   }
  }
  minHeightmapValue = minHeight;
  refreshSectionBlockCountsUnlocked();
 }
 dirty = true;
 if(!fixCrossChunkGaps) {
  return;
 }
 for(int gapX = 0; gapX < 16; ++gapX) {
  for(int gapZ = 0; gapZ < 16; ++gapZ) {
   lightGaps(gapX, gapZ);
  }
 }
}
void Chunk::lightGaps(int localX, int localZ) {
 const int topHeight = getHeight(localX, localZ);
 const int blockX = this->x * 16 + localX;
 const int blockZ = this->z * 16 + localZ;
 lightGap(blockX - 1, blockZ, topHeight);
 lightGap(blockX + 1, blockZ, topHeight);
 lightGap(blockX, blockZ - 1, topHeight);
 lightGap(blockX, blockZ + 1, topHeight);
}
void Chunk::lightGap(int blockX, int blockZ, int yPos) {
 if(world == nullptr) {
  return;
 }
 const int topY = world->getTopY(blockX, blockZ);
 if(topY > yPos) {
  world->queueLightUpdate(LightType::Sky, blockX, yPos, blockZ, blockX, topY, blockZ);
  dirty = true;
 } else if(topY < yPos) {
  world->queueLightUpdate(LightType::Sky, blockX, topY, blockZ, blockX, yPos, blockZ);
  dirty = true;
 }
}
void Chunk::updateHeightMap(int localX, int yPos, int localZ) {
 if(world == nullptr) {
  recalculateHeightColumn(localX, localZ);
  return;
 }
 int oldHeight = static_cast<int>(
     std::atomic_ref(const_cast<std::uint8_t&>(heightmap[static_cast<std::size_t>((localZ << 4) | localX)]))
         .load(std::memory_order_relaxed) &
     0xFFU);
 int newHeight = oldHeight;
 if(yPos > oldHeight) {
  newHeight = yPos;
 }
 const int base = (localX << 11) | (localZ << 7);
 while(newHeight > 0 && Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(
                            std::atomic_ref(const_cast<std::uint8_t&>(
                                                blocks[static_cast<std::size_t>(base + newHeight - 1)]))
                                .load(std::memory_order_relaxed) &
                            0xFFU)] == 0) {
  --newHeight;
 }
 if(newHeight == oldHeight) {
  return;
 }
 world->setBlocksDirtyColumn(this->x * 16 + localX, this->z * 16 + localZ, newHeight, oldHeight);
 const int blockX = this->x * 16 + localX;
 const int blockZ = this->z * 16 + localZ;
 if(newHeight >= oldHeight) {
  world->queueLightUpdate(LightType::Sky, blockX, oldHeight, blockZ, blockX, newHeight, blockZ);
 }
 const int previousTop = newHeight;
 int propagationBottom = newHeight;
 {
  const RenderWriteGuard guard(*this);
  std::atomic_ref<std::uint8_t>(heightmap[static_cast<std::size_t>((localZ << 4) | localX)])
      .store(static_cast<std::uint8_t>(newHeight), std::memory_order_relaxed);
  if(newHeight < minHeightmapValue) {
   minHeightmapValue = newHeight;
  } else {
   int minHeight = 127;
   for(int localZIndex = 0; localZIndex < 16; ++localZIndex) {
    for(int localXIndex = 0; localXIndex < 16; ++localXIndex) {
     const int heightValue = static_cast<int>(
         std::atomic_ref(const_cast<std::uint8_t&>(
                             heightmap[static_cast<std::size_t>((localZIndex << 4) | localXIndex)]))
             .load(std::memory_order_relaxed) &
         0xFFU);
     if(heightValue < minHeight) {
      minHeight = heightValue;
     }
    }
   }
   minHeightmapValue = minHeight;
  }
  if(newHeight < oldHeight) {
   for(int y = newHeight; y < oldHeight; ++y) {
    skyLight.set(localX, y, localZ, 15);
   }
  } else {
   for(int y = oldHeight; y < newHeight; ++y) {
    skyLight.set(localX, y, localZ, 0);
   }
  }
  int light = 15;
  while(propagationBottom > 0 && light > 0) {
   int opacity = Block::BLOCKS_LIGHT_OPACITY[
       static_cast<std::size_t>(getBlockId(localX, --propagationBottom, localZ))];
   if(opacity == 0) {
    opacity = 1;
   }
   light -= opacity;
   if(light < 0) {
    light = 0;
   }
   skyLight.set(localX, propagationBottom, localZ, light);
  }
 }
 while(propagationBottom > 0 &&
       Block::BLOCKS_LIGHT_OPACITY[
           static_cast<std::size_t>(getBlockId(localX, propagationBottom - 1, localZ))] == 0) {
  --propagationBottom;
 }
 if(propagationBottom != previousTop) {
  world->queueLightUpdate(LightType::Sky, blockX - 1, propagationBottom, blockZ - 1,
                          blockX + 1, previousTop, blockZ + 1);
 }
 dirty = true;
}
block::entity::BlockEntity* Chunk::getBlockEntity(int localX, int yPos, int localZ) {
 const std::lock_guard lock(*entityMutex_);
 const Vec3i pos{localX, yPos, localZ};
 auto it = blockEntities.find(pos);
 if(it == blockEntities.end()) {
  const int blockId = getBlockId(localX, yPos, localZ);
  if(blockId <= 0 || blockId >= block::Block::BLOCK_COUNT ||
     !Block::BLOCKS_WITH_ENTITY[static_cast<std::size_t>(blockId)]) {
   return nullptr;
  }
  auto* blockWithEntity =
      dynamic_cast<block::BlockWithEntity*>(Block::BLOCKS[static_cast<std::size_t>(blockId)]);
  if(blockWithEntity == nullptr || world == nullptr) {
   return nullptr;
  }
  blockWithEntity->onPlaced(world, x * 16 + localX, yPos, z * 16 + localZ);
  it = blockEntities.find(pos);
  if(it == blockEntities.end()) {
   return nullptr;
  }
 }
 block::entity::BlockEntity* blockEntity = it->second.get();
 if(blockEntity != nullptr && blockEntity->isRemoved()) {
  blockEntities.erase(it);
  return nullptr;
 }
 return blockEntity;
}
void Chunk::setBlockEntity(int localX, int yPos, int localZ, std::unique_ptr<block::entity::BlockEntity> blockEntity) {
 if(!blockEntity) {
  return;
 }
 const std::lock_guard lock(*entityMutex_);
 const Vec3i pos{localX, yPos, localZ};
 const auto existing = blockEntities.find(pos);
 if(existing != blockEntities.end()) {
  block::entity::BlockEntity* previous = existing->second.get();
  if(previous != nullptr && world != nullptr) {
   auto& list = world->blockEntities;
   list.erase(std::remove(list.begin(), list.end(), previous), list.end());
   previous->markRemoved();
  }
  blockEntities.erase(existing);
 }
 blockEntity->world = world;
 blockEntity->x = x * 16 + localX;
 blockEntity->y = yPos;
 blockEntity->z = z * 16 + localZ;
 blockEntity->cancelRemoval();
 block::entity::BlockEntity* raw = blockEntity.get();
 blockEntities[pos] = std::move(blockEntity);
 if(loaded && world != nullptr && raw != nullptr) {
  auto& list = world->blockEntities;
  if(std::find(list.begin(), list.end(), raw) == list.end()) {
   list.push_back(raw);
  }
 }
}
void Chunk::removeBlockEntityAt(int localX, int yPos, int localZ) {
 const std::lock_guard lock(*entityMutex_);
 const auto it = blockEntities.find(Vec3i{localX, yPos, localZ});
 if(!loaded || it == blockEntities.end()) {
  return;
 }
 block::entity::BlockEntity* blockEntity = it->second.get();
 if(blockEntity != nullptr && world != nullptr) {
  auto& list = world->blockEntities;
  list.erase(std::remove(list.begin(), list.end(), blockEntity), list.end());
  blockEntity->markRemoved();
 }
 blockEntities.erase(it);
}
void Chunk::load() {
 loaded = true;
 if(world == nullptr) {
  return;
 }
 world->registerChunkForLighting(this);
 {
  const std::lock_guard lock(*entityMutex_);
  std::vector<block::entity::BlockEntity*> loadedBlockEntities;
  loadedBlockEntities.reserve(blockEntities.size());
  for(auto& entry : blockEntities) {
   if(entry.second) {
    loadedBlockEntities.push_back(entry.second.get());
   }
  }
  world->processBlockUpdates(loadedBlockEntities);
  for(auto& slice : entities) {
   if(!slice.empty()) {
    world->addEntities(slice);
   }
  }
 }
}
void Chunk::unload() {
 loaded = false;
 if(world != nullptr) {
  // The lighting thread's leases keep the object alive; eviction tombstones the
  // chunk and defers destruction until the leases drain, so freeing the chunk
  // right after unload() returns is safe only for the cache's graveyard sweep.
  world->unregisterChunkForLighting(this);
  {
   const std::lock_guard lock(*entityMutex_);
   auto& list = world->blockEntities;
   for(auto& entry : blockEntities) {
    if(entry.second == nullptr) {
     continue;
    }
    block::entity::BlockEntity* blockEntity = entry.second.get();
    list.erase(std::remove(list.begin(), list.end(), blockEntity), list.end());
    blockEntity->markRemoved();
   }
   for(auto& slice : entities) {
    if(!slice.empty()) {
     world->unloadEntities(slice);
    }
   }
  }
  return;
 }
 {
  const std::lock_guard lock(*entityMutex_);
  for(auto& entry : blockEntities) {
   if(entry.second) {
    entry.second->markRemoved();
   }
  }
 }
}
} // namespace net::minecraft
