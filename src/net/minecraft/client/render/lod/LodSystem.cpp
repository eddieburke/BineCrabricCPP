#include "net/minecraft/client/render/lod/LodSystem.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/lod/LodBlockColors.hpp"
#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"
#include "net/minecraft/world/chunk/storage/RegionIo.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
namespace net::minecraft::client::render::lod {
namespace {
constexpr int kMaxJobsInFlight = 24;
constexpr int kMaxEnqueuePerFrame = 8;
constexpr int kLiveScanPerFrame = 256;
constexpr int kImportMergesPerFrame = 256;
constexpr std::size_t kImportQueueSoftCap = 8192;
[[nodiscard]] std::uint64_t regionKeyOf(int regionX, int regionZ) noexcept {
  return packChunkKey(regionX, regionZ);
}
[[nodiscard]] std::uint64_t chunkSignature(const net::minecraft::Chunk& chunk) noexcept {
  std::uint64_t hash = chunk.terrainPopulated ? 0x9E3779B97F4A7C15ULL : 0xC2B2AE3D27D4EB4FULL;
  for(const std::uint8_t height : chunk.heightmap) {
    hash = (hash ^ static_cast<std::uint64_t>(height)) * 0x100000001B3ULL;
  }
  return hash;
}
[[nodiscard]] int floorDivInt(int value, int divisor) noexcept {
  return net::minecraft::MathHelper::floorDiv(value, divisor);
}
[[nodiscard]] float regionCameraDistance(int regionX, int regionZ, double camX, double camZ) noexcept {
  const double centerX = (static_cast<double>(regionX) + 0.5) * kRegionBlocks;
  const double centerZ = (static_cast<double>(regionZ) + 0.5) * kRegionBlocks;
  const double dx = std::abs(camX - centerX);
  const double dz = std::abs(camZ - centerZ);
  return static_cast<float>(std::max(0.0, std::max(dx, dz) - kRegionBlocks * 0.5));
}
} // namespace
LodSystem& LodSystem::instance() {
  static LodSystem system;
  return system;
}
LodSystem::~LodSystem() {
  stopImport();
}
void LodSystem::setWorld(net::minecraft::World* world) {
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
void LodSystem::onBlockChanged(int x, int /*y*/, int z) {
  if(world_ == nullptr || !enabled_) {
    return;
  }
  const std::uint64_t key = packChunkKey(x >> 4, z >> 4);
  const std::lock_guard lock(dirtyMutex_);
  if(dirtyChunks_.size() < 4096) {
    dirtyChunks_.push_back(key);
  }
}
bool LodSystem::activeForRender() const noexcept {
  return enabled_ && world_ != nullptr && !worldIsNether_;
}
bool LodSystem::fogExtendActive() const noexcept {
  return activeForRender() && fogExtend_;
}
int LodSystem::levelForDistance(float blocks) const noexcept {
  float threshold = 768.0f;
  if(detail_ < 0) {
    threshold *= 0.5f;
  } else if(detail_ > 0) {
    threshold *= 2.0f;
  }
  int level = 0;
  while(level < kMaxLodLevel && blocks >= threshold) {
    threshold *= 2.0f;
    ++level;
  }
  return level;
}
void LodSystem::clearAll() {
  meshWorkers_.cancelAll();
  jobsInFlight_ = 0;
  chunks_.clear();
  liveIngested_.clear();
  regions_.clear();
  groups_.clear();
  scanCursor_ = 0;
  evictCounter_ = 0;
  drawnRegions_ = 0;
  importedChunks_.store(0);
  {
    const std::lock_guard lock(dirtyMutex_);
    dirtyChunks_.clear();
  }
  {
    const std::lock_guard lock(importMutex_);
    importQueue_.clear();
  }
}
void LodSystem::stopImport() {
  importCancel_.store(true);
  if(importThread_.joinable()) {
    importThread_.join();
  }
  importCancel_.store(false);
  importRunning_.store(false);
}
void LodSystem::requestReimport() {
  reimportRequested_.store(true);
}
void LodSystem::requestClear() {
  clearRequested_.store(true);
}
void LodSystem::startImport(double camX, double camZ) {
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
  importThread_ = std::thread(&LodSystem::importWorker, this, worldDir_, camChunkX, camChunkZ);
}
void LodSystem::importWorker(std::filesystem::path worldDir, int camChunkX, int camChunkZ) {
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
  std::vector<std::pair<std::uint64_t, LodChunk>> batch;
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
          LodChunk data;
          extractLodChunk(chunk, data);
          batch.emplace_back(packChunkKey(chunkX, chunkZ), data);
          importedChunks_.fetch_add(1);
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
void LodSystem::drainImport() {
  std::vector<std::pair<std::uint64_t, LodChunk>> pending;
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
void LodSystem::ingestChunk(const net::minecraft::Chunk& chunk) {
  LodChunk data;
  extractLodChunk(chunk, data);
  const std::uint64_t key = packChunkKey(chunk.x, chunk.z);
  liveIngested_[key] = chunkSignature(chunk);
  mergeColumns(key, data, false);
}
void LodSystem::mergeColumns(std::uint64_t key, const LodChunk& data, bool fromImport) {
  if(fromImport && liveIngested_.count(key) != 0) {
    return;
  }
  const int chunkX = chunkKeyX(key);
  const int chunkZ = chunkKeyZ(key);
  const auto [it, inserted] = chunks_.insert_or_assign(key, data);
  (void)it;
  const int regionX = floorDivInt(chunkX, kRegionChunks);
  const int regionZ = floorDivInt(chunkZ, kRegionChunks);
  Region& region = regions_[regionKeyOf(regionX, regionZ)];
  if(inserted) {
    ++region.chunkCount;
  }
  ++region.dataStamp;
  const int localX = chunkX - regionX * kRegionChunks;
  const int localZ = chunkZ - regionZ * kRegionChunks;
  if(localX == 0) {
    markRegionDirty(regionX - 1, regionZ);
  }
  if(localX == kRegionChunks - 1) {
    markRegionDirty(regionX + 1, regionZ);
  }
  if(localZ == 0) {
    markRegionDirty(regionX, regionZ - 1);
  }
  if(localZ == kRegionChunks - 1) {
    markRegionDirty(regionX, regionZ + 1);
  }
}
void LodSystem::markRegionDirty(int regionX, int regionZ) {
  const auto it = regions_.find(regionKeyOf(regionX, regionZ));
  if(it != regions_.end()) {
    ++it->second.dataStamp;
  }
}
void LodSystem::liveScan(double camX, double camZ, int residentChunkRadius) {
  if(world_ == nullptr) {
    return;
  }
  std::vector<std::uint64_t> dirty;
  {
    const std::lock_guard lock(dirtyMutex_);
    dirty.swap(dirtyChunks_);
  }
  for(const std::uint64_t key : dirty) {
    const net::minecraft::Chunk* chunk = world_->getChunkIfLoaded(chunkKeyX(key) * 16, chunkKeyZ(key) * 16);
    if(chunk != nullptr && !chunk->isEmpty()) {
      ingestChunk(*chunk);
    }
  }
  const int radius = std::max(2, residentChunkRadius);
  const int side = radius * 2 + 1;
  const int total = side * side;
  const int camChunkX = floorDivInt(net::minecraft::MathHelper::floor(camX), 16);
  const int camChunkZ = floorDivInt(net::minecraft::MathHelper::floor(camZ), 16);
  for(int i = 0; i < kLiveScanPerFrame; ++i) {
    scanCursor_ = (scanCursor_ + 1) % total;
    const int chunkX = camChunkX + (scanCursor_ % side) - radius;
    const int chunkZ = camChunkZ + (scanCursor_ / side) - radius;
    const net::minecraft::Chunk* chunk = world_->getChunkIfLoaded(chunkX * 16, chunkZ * 16);
    if(chunk == nullptr || chunk->isEmpty()) {
      continue;
    }
    const std::uint64_t key = packChunkKey(chunkX, chunkZ);
    const auto known = liveIngested_.find(key);
    if(known != liveIngested_.end() && known->second == chunkSignature(*chunk) &&
       chunks_.count(key) != 0) {
      continue;
    }
    ingestChunk(*chunk);
  }
}
void LodSystem::scheduleMeshes(double camX, double camZ) {
  const auto& colors = LodBlockColors::table();
  const auto& sideColors = LodBlockColors::sideTable();
  int budget = kMaxEnqueuePerFrame;
  for(auto& [key, region] : regions_) {
    if(budget <= 0 || jobsInFlight_ >= kMaxJobsInFlight) {
      break;
    }
    if(region.jobPending || region.chunkCount <= 0) {
      continue;
    }
    const int regionX = chunkKeyX(key);
    const int regionZ = chunkKeyZ(key);
    const float dist = regionCameraDistance(regionX, regionZ, camX, camZ);
    if(dist > distance_ || dist < nearCutoffBlocks_) {
      continue;
    }
    const int desired = levelForDistance(dist);
    bool levelOk = region.meshedLevel == desired;
    if(!levelOk && region.hasMesh) {
      levelOk = region.meshedLevel == levelForDistance(dist * 0.85f) ||
                region.meshedLevel == levelForDistance(dist * 1.15f);
    }
    if(region.meshStamp == region.dataStamp && levelOk) {
      continue;
    }
    auto job = std::make_shared<LodMeshJob>();
    job->regionX = regionX;
    job->regionZ = regionZ;
    job->level = levelOk && region.hasMesh ? region.meshedLevel : desired;
    job->stamp = region.dataStamp;
    job->colors[0] = &colors;
    job->colors[1] = &sideColors;
    const int groupX = floorDivInt(regionX, kGroupRegions);
    const int groupZ = floorDivInt(regionZ, kGroupRegions);
    job->originX = static_cast<float>(regionX * kRegionBlocks - groupX * kGroupBlocks);
    job->originZ = static_cast<float>(regionZ * kRegionBlocks - groupZ * kGroupBlocks);
    const int span = kRegionChunks + 2;
    job->chunks.resize(static_cast<std::size_t>(span * span));
    job->present.assign(static_cast<std::size_t>(span * span), 0);
    for(int cz = -1; cz <= kRegionChunks; ++cz) {
      for(int cx = -1; cx <= kRegionChunks; ++cx) {
        const std::uint64_t chunkKey =
            packChunkKey(regionX * kRegionChunks + cx, regionZ * kRegionChunks + cz);
        const auto it = chunks_.find(chunkKey);
        if(it == chunks_.end()) {
          continue;
        }
        const std::size_t index = static_cast<std::size_t>((cz + 1) * span + (cx + 1));
        job->chunks[index] = it->second;
        job->present[index] = 1;
      }
    }
    region.jobPending = true;
    ++jobsInFlight_;
    --budget;
    meshWorkers_.enqueue(
        job,
        [](LodMeshJob& meshJob) {
          try {
            LodMeshJob::build(meshJob);
          } catch(...) {
            meshJob.failed = true;
          }
        },
        static_cast<int>(dist));
  }
}
void LodSystem::drainMeshes() {
  const std::vector<std::shared_ptr<LodMeshJob>> completed = meshWorkers_.drainCompleted();
  for(const std::shared_ptr<LodMeshJob>& job : completed) {
    --jobsInFlight_;
    const auto regionIt = regions_.find(regionKeyOf(job->regionX, job->regionZ));
    if(regionIt == regions_.end()) {
      continue;
    }
    Region& region = regionIt->second;
    region.jobPending = false;
    if(job->failed) {
      continue;
    }
    const int groupX = floorDivInt(job->regionX, kGroupRegions);
    const int groupZ = floorDivInt(job->regionZ, kGroupRegions);
    const std::uint64_t groupKey = packChunkKey(groupX, groupZ);
    if(job->vertices.empty()) {
      if(region.hasMesh) {
        const auto groupIt = groups_.find(groupKey);
        if(groupIt != groups_.end()) {
          groupIt->second->buffer.release(region.slot);
          --groupIt->second->meshCount;
          if(groupIt->second->meshCount <= 0) {
            groups_.erase(groupIt);
          }
        }
        region.hasMesh = false;
      }
    } else {
      std::unique_ptr<Group>& group = groups_[groupKey];
      if(group == nullptr) {
        group = std::make_unique<Group>();
        group->originX = static_cast<double>(groupX) * kGroupBlocks;
        group->originZ = static_cast<double>(groupZ) * kGroupBlocks;
      }
      group->buffer.upload(region.slot,
                           job->vertices.data(),
                           static_cast<int>(job->vertices.size()),
                           true,
                           true,
                           false);
      if(!region.hasMesh) {
        ++group->meshCount;
      }
      region.hasMesh = true;
      region.minY = job->minY;
      region.maxY = job->maxY;
    }
    region.meshedLevel = job->level;
    region.meshStamp = job->stamp;
  }
}
void LodSystem::evict(double camX, double camZ) {
  if(++evictCounter_ < 32) {
    return;
  }
  evictCounter_ = 0;
  const float limit = distance_ * 1.25f + 128.0f;
  for(auto& [key, region] : regions_) {
    if(!region.hasMesh) {
      continue;
    }
    const int regionX = chunkKeyX(key);
    const int regionZ = chunkKeyZ(key);
    const float dist = regionCameraDistance(regionX, regionZ, camX, camZ);
    if(dist <= limit && dist >= nearCutoffBlocks_) {
      continue;
    }
    const std::uint64_t groupKey =
        packChunkKey(floorDivInt(regionX, kGroupRegions), floorDivInt(regionZ, kGroupRegions));
    const auto groupIt = groups_.find(groupKey);
    if(groupIt != groups_.end()) {
      groupIt->second->buffer.release(region.slot);
      --groupIt->second->meshCount;
      if(groupIt->second->meshCount <= 0) {
        groups_.erase(groupIt);
      }
    }
    region.hasMesh = false;
    region.meshedLevel = -1;
    region.meshStamp = 0;
  }
}
void LodSystem::updateStats() {
  const std::lock_guard lock(statsMutex_);
  stats_.enabled = enabled_;
  stats_.importing = importRunning_.load();
  stats_.distance = distance_;
  stats_.chunks = static_cast<long long>(chunks_.size());
  stats_.regions = static_cast<long long>(regions_.size());
  long long meshed = 0;
  for(const auto& [key, region] : regions_) {
    if(region.hasMesh) {
      ++meshed;
    }
  }
  stats_.meshedRegions = meshed;
  stats_.pendingJobs = jobsInFlight_;
  stats_.drawnRegions = drawnRegions_;
  stats_.importedChunks = importedChunks_.load();
}
LodStats LodSystem::statsSnapshot() const {
  const std::lock_guard lock(statsMutex_);
  return stats_;
}
void LodSystem::frameUpdate(double camX,
                            double camY,
                            double camZ,
                            const option::ResolvedRenderOptions& resolved) {
  (void)camY;
  setEnabled(resolved.lodEnabled);
  setDistance(resolved.lodDistanceBlocks);
  setDetail(resolved.lodDetail);
  setFogExtend(resolved.lodFogExtend);
  setImportWorld(resolved.lodImportWorld);
  // LOD terrain must never draw inside the radius vanilla already renders in
  // full detail, or the coarse LOD quads z-fight and poke through real
  // terrain. Keep a half-region overlap (rather than a hard edge) so a
  // shrinking vanilla grid can't open a gap before LOD catches up.
  nearCutoffBlocks_ = std::max(0.0f, static_cast<float>(resolved.chunkRadius) * 16.0f - kRegionBlocks * 0.5f);
  if(clearRequested_.exchange(false)) {
    stopImport();
    clearAll();
    importStartedForWorld_ = false;
  }
  if(world_ == nullptr) {
    return;
  }
  if(!enabled_) {
    updateStats();
    return;
  }
  if(reimportRequested_.exchange(false)) {
    stopImport();
    liveIngested_.clear();
    startImport(camX, camZ);
  }
  if(importWorld_ && !importStartedForWorld_ && !worldDir_.empty()) {
    startImport(camX, camZ);
  }
  drainImport();
  liveScan(camX, camZ, resolved.residentChunkRadius);
  scheduleMeshes(camX, camZ);
  drainMeshes();
  evict(camX, camZ);
  updateStats();
}
void LodSystem::render(double camX, double camY, double camZ, float brightness, bool frustumCull) {
  drawnRegions_ = 0;
  if(!activeForRender() || groups_.empty()) {
    return;
  }
  if(brightnessTexture_ == 0) {
    gl::genTextures(1, &brightnessTexture_);
    gl::bindTexture(gl::cap::Texture2D, static_cast<int>(brightnessTexture_));
    gl::texParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Nearest);
    gl::texParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Nearest);
    const std::uint8_t white[4] = {255, 255, 255, 255};
    gl::texImage2D(gl::cap::Texture2D, 0, gl::pixel::Rgba, 1, 1, 0, gl::pixel::Rgba, gl::pixel::UnsignedByte, white);
    lastBrightness_ = 1.0f;
  } else {
    gl::bindTexture(gl::cap::Texture2D, static_cast<int>(brightnessTexture_));
  }
  if(std::abs(brightness - lastBrightness_) > 0.004f) {
    const std::uint8_t value = static_cast<std::uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255.0f);
    const std::uint8_t texel[4] = {value, value, value, 255};
    gl::texSubImage2D(gl::cap::Texture2D, 0, 0, 0, 1, 1, gl::pixel::Rgba, gl::pixel::UnsignedByte, texel);
    lastBrightness_ = brightness;
  }
  FrustumCuller culler;
  if(frustumCull) {
    culler.prepare(camX, camY, camZ);
  }
  gl::setCap(gl::cap::Texture2D, true);
  gl::setCap(gl::cap::CullFace, false);
  gl::setCap(gl::cap::Blend, false);
  gl::setCap(gl::cap::AlphaTest, false);
  gl::depthMask(true);
  gl::setCap(gl::cap::PolygonOffsetFill, true);
  gl::polygonOffset(1.0f, 2.0f);
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  for(auto& [groupKey, group] : groups_) {
    group->buffer.beginFrame();
  }
  for(auto& [key, region] : regions_) {
    if(!region.hasMesh || !region.slot.valid()) {
      continue;
    }
    const int regionX = chunkKeyX(key);
    const int regionZ = chunkKeyZ(key);
    const float dist = regionCameraDistance(regionX, regionZ, camX, camZ);
    if(dist > distance_ || dist < nearCutoffBlocks_) {
      continue;
    }
    if(frustumCull) {
      const net::minecraft::Box box{static_cast<double>(regionX) * kRegionBlocks,
                                    static_cast<double>(region.minY) - 1.0,
                                    static_cast<double>(regionZ) * kRegionBlocks,
                                    static_cast<double>(regionX + 1) * kRegionBlocks,
                                    static_cast<double>(region.maxY) + 1.0,
                                    static_cast<double>(regionZ + 1) * kRegionBlocks};
      if(!culler.isVisible(box)) {
        continue;
      }
    }
    const std::uint64_t groupKey =
        packChunkKey(floorDivInt(regionX, kGroupRegions), floorDivInt(regionZ, kGroupRegions));
    const auto groupIt = groups_.find(groupKey);
    if(groupIt != groups_.end()) {
      groupIt->second->buffer.addVisible(region.slot);
    }
  }
  const int drawMode = Tessellator::effectiveDrawMode(gl::prim::Quads);
  for(auto& [groupKey, group] : groups_) {
    if(!group->buffer.hasVisible()) {
      continue;
    }
    const gl::MatrixGuard matrix;
    gl::translatef(static_cast<float>(group->originX - camX),
                   static_cast<float>(-camY),
                   static_cast<float>(group->originZ - camZ));
    drawnRegions_ += group->buffer.flush(drawMode);
  }
  gl::polygonOffset(0.0f, 0.0f);
  gl::setCap(gl::cap::PolygonOffsetFill, false);
  gl::setCap(gl::cap::CullFace, true);
  gl::setCap(gl::cap::AlphaTest, true);
  gl::alphaFunc(gl::compare::Greater, 0.1f);
}
} // namespace net::minecraft::client::render::lod
