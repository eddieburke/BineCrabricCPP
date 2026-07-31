#pragma once
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/world/chunk/storage/RegionFile.hpp"
namespace net::minecraft {
namespace fs = std::filesystem;
// Access to .mcr region files with per-file locking so loads/saves of different
// regions can proceed in parallel (vanilla Java synchronized each RegionFile).
class RegionIo {
 public:
 static std::optional<std::vector<std::uint8_t>> readChunkData(const fs::path& worldDir, int chunkX, int chunkZ) {
  std::optional<RegionFile::CompressedChunk> chunk;
  {
   const std::shared_ptr<LockedRegion> region = regionHandle(worldDir, chunkX, chunkZ);
   const std::lock_guard lock(region->mutex);
   chunk = region->file.readCompressedChunk(chunkX & 0x1F, chunkZ & 0x1F);
  }
  if(!chunk.has_value()) {
   return std::nullopt;
  }
  if(chunk->compression == 1U) {
   return gzipDecompress(chunk->bytes);
  }
  if(chunk->compression == 2U) {
   return zlibDecompress(chunk->bytes);
  }
  return std::nullopt;
 }
 static void writeChunkData(const fs::path& worldDir,
                            int chunkX,
                            int chunkZ,
                            const std::vector<std::uint8_t>& data) {
  const std::shared_ptr<LockedRegion> region = regionHandle(worldDir, chunkX, chunkZ);
  const std::lock_guard lock(region->mutex);
  region->file.writeChunk(chunkX & 0x1F, chunkZ & 0x1F, data);
 }
 static int getChunkSize(const fs::path& worldDir, int chunkX, int chunkZ) {
  const std::shared_ptr<LockedRegion> region = regionHandle(worldDir, chunkX, chunkZ);
  const std::lock_guard lock(region->mutex);
  return region->file.resetBytesWritten();
 }
 static void flush() {
  std::vector<std::shared_ptr<LockedRegion>> snapshot;
  {
   const std::lock_guard lock(registryMutex());
   snapshot.reserve(openFiles().size());
   for(auto& [key, file] : openFiles()) {
    (void)key;
    snapshot.push_back(file);
   }
   openFiles().clear();
  }
  for(const std::shared_ptr<LockedRegion>& region : snapshot) {
   if(region == nullptr) {
    continue;
   }
   const std::lock_guard lock(region->mutex);
   region->file.flush();
  }
 }
 static void sync() {
  std::vector<std::shared_ptr<LockedRegion>> snapshot;
  {
   const std::lock_guard lock(registryMutex());
   snapshot.reserve(openFiles().size());
   for(auto& [key, file] : openFiles()) {
    (void)key;
    snapshot.push_back(file);
   }
  }
  for(const std::shared_ptr<LockedRegion>& region : snapshot) {
   if(region == nullptr) {
    continue;
   }
   const std::lock_guard lock(region->mutex);
   region->file.flush();
  }
 }

 private:
 struct LockedRegion {
  explicit LockedRegion(fs::path path) : file(std::move(path)) {
  }
  RegionFile file;
  std::mutex mutex;
 };
 static std::shared_ptr<LockedRegion> regionHandle(const fs::path& worldDir, int chunkX, int chunkZ) {
  const fs::path regionDir = worldDir / "region";
  const fs::path regionPath =
      regionDir / ("r." + std::to_string(chunkX >> 5) + "." + std::to_string(chunkZ >> 5) + ".mcr");
  const std::string key = regionPath.lexically_normal().generic_string();
  const std::lock_guard lock(registryMutex());
  auto& slot = openFiles()[key];
  if(!slot) {
   slot = std::make_shared<LockedRegion>(regionPath);
  }
  return slot;
 }
 static std::unordered_map<std::string, std::shared_ptr<LockedRegion>>& openFiles() {
  static std::unordered_map<std::string, std::shared_ptr<LockedRegion>> files;
  return files;
 }
 static std::mutex& registryMutex() {
  static std::mutex mutex;
  return mutex;
 }
};
} // namespace net::minecraft
