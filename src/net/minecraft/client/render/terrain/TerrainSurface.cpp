#include "net/minecraft/client/render/terrain/TerrainSurface.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"
#include "net/minecraft/world/chunk/storage/RegionIo.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
namespace net::minecraft::client::render::terrain {
namespace {
constexpr int kLiveScanPerFrame = 32;
constexpr int kImportMergesPerFrame = 32;
constexpr std::size_t kImportQueueSoftCap = 8192;
constexpr std::size_t kDirtyChunkQueueCap = 4096;
[[nodiscard]] std::uint64_t regionKeyOf(int regionX, int regionZ) noexcept {
  return packChunkKey(regionX, regionZ);
}
[[nodiscard]] int floorDivInt(int value, int divisor) noexcept {
  return net::minecraft::MathHelper::floorDiv(value, divisor);
}
} // namespace
TerrainSurface& TerrainSurface::instance() {
  static TerrainSurface surface;
  return surface;
}
TerrainSurface::~TerrainSurface() {
  stopImport();
}
void TerrainSurface::setWorld(net::minecraft::World* world) {
  if(world == world_) {
    return;
  }
  stopImport();
  clearAll();
  world_ = world;
  worldIsNether_ = false;
  worldDir_.clear();
  importStartedForWorld_ = false;
  if(world_ == nullptr) {
    return;
  }
  worldIsNether_ = world_->dimension != nullptr && world_->dimension->isNether;
  if(!world_->isRemote() && !worldIsNether_ && world_->getDimensionData() != nullptr) {
    std::error_code ec;
    const std::filesystem::path dir = world_->getDimensionData()->worldDirectory();
    if(!dir.empty() && std::filesystem::is_directory(dir / "region", ec)) {
      worldDir_ = dir;
    }
  }
}
void TerrainSurface::onBlockChanged(int x, int /*y*/, int z) {
  onBlocksChanged(x, z, x, z);
}
void TerrainSurface::onBlocksChanged(int minX, int minZ, int maxX, int maxZ) {
  if(world_ == nullptr || !enabled_) {
    return;
  }
  if(minX > maxX) {
    std::swap(minX, maxX);
  }
  if(minZ > maxZ) {
    std::swap(minZ, maxZ);
  }
  const int minChunkX = floorDivInt(minX, 16);
  const int maxChunkX = floorDivInt(maxX, 16);
  const int minChunkZ = floorDivInt(minZ, 16);
  const int maxChunkZ = floorDivInt(maxZ, 16);
  const std::lock_guard lock(dirtyMutex_);
  for(int chunkZ = minChunkZ; chunkZ <= maxChunkZ; ++chunkZ) {
    for(int chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX) {
      const std::uint64_t key = packChunkKey(chunkX, chunkZ);
      if(dirtyChunks_.size() < kDirtyChunkQueueCap && dirtyChunkSet_.insert(key).second) {
        dirtyChunks_.push_back(key);
      }
    }
  }
}
bool TerrainSurface::active() const noexcept {
  return enabled_ && world_ != nullptr && !world_->isRemote() && !worldIsNether_;
}
bool TerrainSurface::fogExtendActive() const noexcept {
  return active() && fogExtend_;
}
void TerrainSurface::clearAll() {
  importedChunkData_.clear();
  liveChunks_.clear();
  liveIngested_.clear();
  liveEmptyChunks_.clear();
  regions_.clear();
  changedRegions_.clear();
  changedRegionSet_.clear();
  scanCursor_ = 0;
  residentChunkCount_ = 0;
  {
    const std::lock_guard lock(dirtyMutex_);
    dirtyChunks_.clear();
    dirtyChunkSet_.clear();
  }
  {
    const std::lock_guard lock(importMutex_);
    importQueue_.clear();
  }
}
void TerrainSurface::stopImport() {
  importCancel_.store(true);
  if(importThread_.joinable()) {
    importThread_.join();
  }
  importCancel_.store(false);
  importRunning_.store(false);
}
void TerrainSurface::requestReimport() {
  reimportRequested_.store(true);
}
void TerrainSurface::requestClear() {
  clearRequested_.store(true);
}
void TerrainSurface::startImport(double camX, double camZ) {
  if(importRunning_.load() || worldDir_.empty()) {
    return;
  }
  if(importThread_.joinable()) {
    importThread_.join();
  }
  importCancel_.store(false);
  importRunning_.store(true);
  importStartedForWorld_ = true;
  const int camChunkX = floorDivInt(net::minecraft::MathHelper::floor(camX), 16);
  const int camChunkZ = floorDivInt(net::minecraft::MathHelper::floor(camZ), 16);
  importThread_ = std::thread(&TerrainSurface::importWorker, this, worldDir_, camChunkX, camChunkZ);
}
void TerrainSurface::importWorker(std::filesystem::path worldDir, int camChunkX, int camChunkZ) {
  struct RegionFileEntry {
    int x = 0;
    int z = 0;
  };
  std::vector<RegionFileEntry> files;
  std::error_code ec;
  for(const auto& entry : std::filesystem::directory_iterator(worldDir / "region", ec)) {
    if(!entry.is_regular_file(ec)) {
      continue;
    }
    const std::string name = entry.path().filename().string();
    if(name.size() < 8 || name.rfind("r.", 0) != 0 || name.substr(name.size() - 4) != ".mcr") {
      continue;
    }
    const std::string middle = name.substr(2, name.size() - 6);
    const std::size_t dot = middle.find('.');
    if(dot == std::string::npos) {
      continue;
    }
    try {
      RegionFileEntry file;
      file.x = std::stoi(middle.substr(0, dot));
      file.z = std::stoi(middle.substr(dot + 1));
      files.push_back(file);
    } catch(const std::exception&) {
    }
  }
  const int camRegionFileX = floorDivInt(camChunkX, 32);
  const int camRegionFileZ = floorDivInt(camChunkZ, 32);
  std::sort(files.begin(), files.end(), [&](const RegionFileEntry& a, const RegionFileEntry& b) {
    const int da = std::max(std::abs(a.x - camRegionFileX), std::abs(a.z - camRegionFileZ));
    const int db = std::max(std::abs(b.x - camRegionFileX), std::abs(b.z - camRegionFileZ));
    return da < db;
  });
  std::vector<std::pair<std::uint64_t, TerrainChunk>> batch;
  const auto flushBatch = [&]() {
    if(batch.empty()) {
      return;
    }
    for(;;) {
      {
        const std::lock_guard lock(importMutex_);
        if(importQueue_.size() < kImportQueueSoftCap) {
          importQueue_.insert(importQueue_.end(),
                              std::make_move_iterator(batch.begin()),
                              std::make_move_iterator(batch.end()));
          batch.clear();
          return;
        }
      }
      if(importCancel_.load()) {
        batch.clear();
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  };
  for(const RegionFileEntry& file : files) {
    for(int localZ = 0; localZ < 32 && !importCancel_.load(); ++localZ) {
      for(int localX = 0; localX < 32; ++localX) {
        if(importCancel_.load()) {
          break;
        }
        const int chunkX = file.x * 32 + localX;
        const int chunkZ = file.z * 32 + localZ;
        const std::optional<std::vector<std::uint8_t>> raw =
            net::minecraft::RegionIo::readChunkData(worldDir, chunkX, chunkZ);
        if(!raw.has_value()) {
          continue;
        }
        try {
          net::minecraft::Nbt root = net::minecraft::Nbt::read(*raw);
          net::minecraft::NbtCompound rootCompound = net::minecraft::NbtCompound::bind(root);
          net::minecraft::Chunk chunk =
              net::minecraft::AlphaChunkStorage::loadChunkFromRootNbt(nullptr, rootCompound, chunkX, chunkZ);
          if(chunk.isEmpty()) {
            continue;
          }
          TerrainChunk data;
          extractTerrainChunk(chunk, data);
          batch.emplace_back(packChunkKey(chunkX, chunkZ), data);
          if(batch.size() >= 64) {
            flushBatch();
          }
        } catch(const std::exception&) {
        }
      }
    }
    if(importCancel_.load()) {
      break;
    }
  }
  flushBatch();
  importRunning_.store(false);
}
void TerrainSurface::drainImport() {
  std::vector<std::pair<std::uint64_t, TerrainChunk>> pending;
  {
    const std::lock_guard lock(importMutex_);
    if(importQueue_.empty()) {
      return;
    }
    if(importQueue_.size() <= kImportMergesPerFrame) {
      pending = std::move(importQueue_);
      importQueue_.clear();
    } else {
      pending.assign(std::make_move_iterator(importQueue_.begin()),
                     std::make_move_iterator(importQueue_.begin() + kImportMergesPerFrame));
      importQueue_.erase(importQueue_.begin(), importQueue_.begin() + kImportMergesPerFrame);
    }
  }
  for(auto& [key, data] : pending) {
    mergeColumns(key, data, true);
  }
}
void TerrainSurface::ingestChunk(const net::minecraft::Chunk& chunk) {
  TerrainChunk data;
  extractTerrainChunk(chunk, data);
  const std::uint64_t key = packChunkKey(chunk.x, chunk.z);
  liveIngested_[key] = 1;
  mergeColumns(key, data, false);
}
const TerrainChunk* TerrainSurface::effectiveChunk(std::uint64_t key) const noexcept {
  const auto live = liveChunks_.find(key);
  if(live != liveChunks_.end()) {
    return &live->second;
  }
  if(liveEmptyChunks_.contains(key)) {
    return nullptr;
  }
  const auto imported = importedChunkData_.find(key);
  if(imported != importedChunkData_.end()) {
    return &imported->second;
  }
  return nullptr;
}
void TerrainSurface::effectiveChunkChanged(std::uint64_t key, bool wasPresent, bool isPresent) {
  const int chunkX = chunkKeyX(key);
  const int chunkZ = chunkKeyZ(key);
  const int regionX = floorDivInt(chunkX, kTerrainRegionChunks);
  const int regionZ = floorDivInt(chunkZ, kTerrainRegionChunks);
  const std::uint64_t regionKey = regionKeyOf(regionX, regionZ);
  Region& region = regions_[regionKey];
  if(!wasPresent && isPresent) {
    ++region.chunkCount;
    ++residentChunkCount_;
  } else if(wasPresent && !isPresent) {
    region.chunkCount = std::max(0, region.chunkCount - 1);
    residentChunkCount_ = residentChunkCount_ == 0 ? 0 : residentChunkCount_ - 1;
  }
  markRegionDirty(regionX, regionZ);
  const int localX = chunkX - regionX * kTerrainRegionChunks;
  const int localZ = chunkZ - regionZ * kTerrainRegionChunks;
  if(localX == 0) {
    markRegionDirty(regionX - 1, regionZ);
  }
  if(localX == kTerrainRegionChunks - 1) {
    markRegionDirty(regionX + 1, regionZ);
  }
  if(localZ == 0) {
    markRegionDirty(regionX, regionZ - 1);
  }
  if(localZ == kTerrainRegionChunks - 1) {
    markRegionDirty(regionX, regionZ + 1);
  }
}
void TerrainSurface::mergeColumns(std::uint64_t key, const TerrainChunk& data, bool fromImport) {
  const TerrainChunk* previousChunk = effectiveChunk(key);
  const bool wasPresent = previousChunk != nullptr;
  const TerrainChunk previous = wasPresent ? *previousChunk : TerrainChunk{};
  if(fromImport) {
    importedChunkData_.insert_or_assign(key, data);
    if(liveChunks_.contains(key) || liveEmptyChunks_.contains(key)) {
      return;
    }
  } else {
    liveEmptyChunks_.erase(key);
    liveChunks_.insert_or_assign(key, data);
  }
  const TerrainChunk* current = effectiveChunk(key);
  const bool isPresent = current != nullptr;
  if(wasPresent == isPresent && (!isPresent || previous == *current)) {
    return;
  }
  effectiveChunkChanged(key, wasPresent, isPresent);
}
void TerrainSurface::markLiveEmpty(std::uint64_t key) {
  const bool hadLiveChunk = liveChunks_.contains(key);
  const bool hadEmptyMarker = liveEmptyChunks_.contains(key);
  if(hadEmptyMarker && !hadLiveChunk) {
    return;
  }
  const bool wasPresent = effectiveChunk(key) != nullptr;
  liveChunks_.erase(key);
  liveIngested_.erase(key);
  liveEmptyChunks_.insert(key);
  if(hadLiveChunk || wasPresent) {
    effectiveChunkChanged(key, wasPresent, false);
  }
}
void TerrainSurface::clearLiveOverride(std::uint64_t key) {
  const bool hadLiveChunk = liveChunks_.contains(key);
  const bool hadEmptyMarker = liveEmptyChunks_.contains(key);
  if(!hadLiveChunk && !hadEmptyMarker) {
    return;
  }
  const bool wasPresent = effectiveChunk(key) != nullptr;
  liveChunks_.erase(key);
  liveIngested_.erase(key);
  liveEmptyChunks_.erase(key);
  const bool isPresent = effectiveChunk(key) != nullptr;
  if(hadLiveChunk || wasPresent != isPresent) {
    effectiveChunkChanged(key, wasPresent, isPresent);
  }
}
void TerrainSurface::markRegionDirty(int regionX, int regionZ) {
  const std::uint64_t key = regionKeyOf(regionX, regionZ);
  const auto it = regions_.find(key);
  if(it != regions_.end()) {
    ++it->second.dataStamp;
    if(changedRegionSet_.insert(key).second) {
      changedRegions_.push_back(key);
    }
  }
}
void TerrainSurface::liveScan(double camX, double camZ, int residentChunkRadius) {
  if(world_ == nullptr) {
    return;
  }
  std::vector<std::uint64_t> dirty;
  {
    const std::lock_guard lock(dirtyMutex_);
    dirty.swap(dirtyChunks_);
    dirtyChunkSet_.clear();
  }
  for(const std::uint64_t key : dirty) {
    const net::minecraft::Chunk* chunk = world_->getChunkIfLoaded(chunkKeyX(key) * 16, chunkKeyZ(key) * 16);
    if(chunk == nullptr) {
      clearLiveOverride(key);
    } else if(chunk->isEmpty()) {
      markLiveEmpty(key);
    } else {
      ingestChunk(*chunk);
    }
  }
  const int camChunkX = floorDivInt(net::minecraft::MathHelper::floor(camX), 16);
  const int camChunkZ = floorDivInt(net::minecraft::MathHelper::floor(camZ), 16);
  const int radius = std::max(2, residentChunkRadius);
  const int side = radius * 2 + 1;
  const int total = side * side;
  for(int i = 0; i < kLiveScanPerFrame; ++i) {
    scanCursor_ = (scanCursor_ + 1) % total;
    const int chunkX = camChunkX + (scanCursor_ % side) - radius;
    const int chunkZ = camChunkZ + (scanCursor_ / side) - radius;
    const net::minecraft::Chunk* chunk = world_->getChunkIfLoaded(chunkX * 16, chunkZ * 16);
    const std::uint64_t key = packChunkKey(chunkX, chunkZ);
    if(chunk == nullptr) {
      clearLiveOverride(key);
      continue;
    }
    if(chunk->isEmpty()) {
      markLiveEmpty(key);
      continue;
    }
    if(liveIngested_.contains(key) && liveChunks_.contains(key)) {
      continue;
    }
    ingestChunk(*chunk);
  }
}
long long TerrainSurface::terrainRegionStamp(int regionX, int regionZ) const noexcept {
  const auto it = regions_.find(regionKeyOf(regionX, regionZ));
  if(it == regions_.end() || it->second.chunkCount <= 0) {
    return -1;
  }
  return it->second.dataStamp;
}
bool TerrainSurface::terrainRegionSnapshot(int regionX, int regionZ, TerrainRegionSnapshot& out) const {
  const long long stamp = terrainRegionStamp(regionX, regionZ);
  if(stamp < 0) {
    out = {};
    return false;
  }
  constexpr int span = kTerrainRegionChunks + 2;
  constexpr std::size_t columnBytes = kTerrainColumnBytes;
  constexpr std::size_t chunkBytes = kTerrainChunkBytes;
  out.stamp = stamp;
  out.bytes.assign(static_cast<std::size_t>(span * span) * chunkBytes, 0);
  std::size_t offset = 0;
  for(int cz = -1; cz <= kTerrainRegionChunks; ++cz) {
    for(int cx = -1; cx <= kTerrainRegionChunks; ++cx) {
      const TerrainChunk* chunk = effectiveChunk(
          packChunkKey(regionX * kTerrainRegionChunks + cx, regionZ * kTerrainRegionChunks + cz));
      out.bytes[offset++] = chunk != nullptr ? 1 : 0;
      if(chunk == nullptr) {
        offset += 256 * columnBytes;
        continue;
      }
      for(const TerrainColumn& column : chunk->columns) {
        out.bytes[offset++] = column.topY;
        out.bytes[offset++] = column.topBlock;
        out.bytes[offset++] = column.groundBlock;
        out.bytes[offset++] = column.waterY;
      }
    }
  }
  return true;
}
TerrainRenderSettings TerrainSurface::terrainRenderSettings() const noexcept {
  return {enabled_, distance_, nearCutoffBlocks_, brightness_, detail_};
}
std::vector<TerrainRegionChange> TerrainSurface::drainChangedRegions(std::size_t maxRegions) {
  std::vector<TerrainRegionChange> changes;
  const std::size_t count = std::min(maxRegions, changedRegions_.size());
  changes.reserve(count);
  for(std::size_t index = 0; index < count; ++index) {
    const std::uint64_t key = changedRegions_.front();
    changedRegions_.pop_front();
    changedRegionSet_.erase(key);
    const int regionX = chunkKeyX(key);
    const int regionZ = chunkKeyZ(key);
    changes.push_back({regionX, regionZ, terrainRegionStamp(regionX, regionZ)});
  }
  return changes;
}
void TerrainSurface::update(double camX,
                            double camY,
                            double camZ,
                            float tickDelta,
                            const option::ResolvedRenderOptions& resolved) {
  (void)camY;
  const float requestedDistance = std::clamp(resolved.lodDistanceBlocks, 256.0f, 8192.0f);
  const int requestedDetail = std::clamp(resolved.lodDetail, -1, 1);
  const float terrainEdge = std::max(0.0f, static_cast<float>(resolved.chunkRadius) * 16.0f - 8.0f);
  const float requestedNearCutoff = std::min(resolved.renderDistanceBlocks, terrainEdge);
  setEnabled(resolved.lodEnabled && world_ != nullptr && !world_->isRemote());
  setDistance(requestedDistance);
  setDetail(requestedDetail);
  setFogExtend(resolved.lodFogExtend);
  setImportWorld(resolved.lodImportWorld);
  nearCutoffBlocks_ = requestedNearCutoff;
  brightness_ = 1.0f;
  if(world_ != nullptr) {
    float daylight = MathHelper::cos(world_->getTime(tickDelta) * 3.14159265f * 2.0f) * 2.0f + 0.5f;
    daylight = std::clamp(daylight, 0.0f, 1.0f);
    const float rainDim = 1.0f - option::rainGradient(resolved, world_, tickDelta) * 0.3f;
    brightness_ = (0.22f + 0.78f * daylight) * rainDim;
  }
  if(clearRequested_.exchange(false)) {
    stopImport();
    clearAll();
    importStartedForWorld_ = false;
  }
  if(world_ == nullptr || !enabled_) {
    return;
  }
  if(reimportRequested_.exchange(false)) {
    stopImport();
    clearAll();
    importStartedForWorld_ = false;
    startImport(camX, camZ);
  }
  if(importWorld_ && !importStartedForWorld_ && !worldDir_.empty()) {
    startImport(camX, camZ);
  }
  drainImport();
  liveScan(camX, camZ, resolved.residentChunkRadius);
}
} // namespace net::minecraft::client::render::terrain
