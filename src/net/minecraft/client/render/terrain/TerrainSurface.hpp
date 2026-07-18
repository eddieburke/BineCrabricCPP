#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/terrain/TerrainSurfaceData.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::client::render::terrain {
struct TerrainRegionSnapshot {
  long long stamp = 0;
  std::vector<std::uint8_t> bytes{};
};
struct TerrainRenderSettings {
  bool enabled = false;
  float distance = 0.0f;
  float nearCutoff = 0.0f;
  float brightness = 1.0f;
  int detail = 0;
};
struct TerrainRegionChange {
  int regionX = 0;
  int regionZ = 0;
  long long stamp = -1;
};
class TerrainSurface {
public:
  static TerrainSurface& instance();
  TerrainSurface(const TerrainSurface&) = delete;
  TerrainSurface& operator=(const TerrainSurface&) = delete;
  void setWorld(net::minecraft::World* world);
  void onBlockChanged(int x, int y, int z);
  void onBlocksChanged(int minX, int minZ, int maxX, int maxZ);
  void setEnabled(bool enabled) noexcept {
    enabled_ = enabled;
  }
  void setDistance(float blocks) noexcept {
    distance_ = std::clamp(blocks, 256.0f, 8192.0f);
  }
  void setDetail(int detail) noexcept {
    detail_ = std::clamp(detail, -1, 1);
  }
  void setFogExtend(bool extend) noexcept {
    fogExtend_ = extend;
  }
  void setImportWorld(bool importWorld) noexcept {
    importWorld_ = importWorld;
  }
  [[nodiscard]] bool active() const noexcept;
  [[nodiscard]] float viewDistanceBlocks() const noexcept {
    return distance_;
  }
  [[nodiscard]] bool fogExtendActive() const noexcept;
  void update(double camX,
              double camY,
              double camZ,
              float tickDelta,
              const option::ResolvedRenderOptions& resolved);
  void requestReimport();
  void requestClear();
  [[nodiscard]] bool terrainRegionSnapshot(int regionX, int regionZ, TerrainRegionSnapshot& out) const;
  [[nodiscard]] long long terrainRegionStamp(int regionX, int regionZ) const noexcept;
  [[nodiscard]] std::vector<TerrainRegionChange> drainChangedRegions(std::size_t maxRegions);
  [[nodiscard]] TerrainRenderSettings terrainRenderSettings() const noexcept;

private:
  TerrainSurface() = default;
  ~TerrainSurface();
  struct Region {
    long long dataStamp = 1;
    int chunkCount = 0;
  };
  void clearAll();
  void stopImport();
  void startImport(double camX, double camZ);
  void importWorker(std::filesystem::path worldDir, int camChunkX, int camChunkZ);
  void drainImport();
  void liveScan(double camX, double camZ, int residentChunkRadius);
  void ingestChunk(const net::minecraft::Chunk& chunk);
  void mergeColumns(std::uint64_t key, const TerrainChunk& data, bool fromImport);
  void markLiveEmpty(std::uint64_t key);
  void clearLiveOverride(std::uint64_t key);
  void effectiveChunkChanged(std::uint64_t key, bool wasPresent, bool isPresent);
  [[nodiscard]] const TerrainChunk* effectiveChunk(std::uint64_t key) const noexcept;
  void markRegionDirty(int regionX, int regionZ);
  net::minecraft::World* world_ = nullptr;
  bool worldIsNether_ = false;
  std::filesystem::path worldDir_{};
  bool enabled_ = true;
  float distance_ = 2048.0f;
  int detail_ = 0;
  bool fogExtend_ = true;
  bool importWorld_ = true;
  float nearCutoffBlocks_ = 0.0f;
  float brightness_ = 1.0f;
  std::unordered_map<std::uint64_t, TerrainChunk> importedChunkData_{};
  std::unordered_map<std::uint64_t, TerrainChunk> liveChunks_{};
  std::unordered_map<std::uint64_t, std::uint64_t> liveIngested_{};
  std::unordered_set<std::uint64_t> liveEmptyChunks_{};
  std::unordered_map<std::uint64_t, Region> regions_{};
  std::deque<std::uint64_t> changedRegions_{};
  std::unordered_set<std::uint64_t> changedRegionSet_{};
  int scanCursor_ = 0;
  std::size_t residentChunkCount_ = 0;
  std::mutex dirtyMutex_{};
  std::vector<std::uint64_t> dirtyChunks_{};
  std::unordered_set<std::uint64_t> dirtyChunkSet_{};
  std::thread importThread_{};
  std::atomic<bool> importRunning_{false};
  std::atomic<bool> importCancel_{false};
  bool importStartedForWorld_ = false;
  std::mutex importMutex_{};
  std::vector<std::pair<std::uint64_t, TerrainChunk>> importQueue_{};
  std::atomic<bool> reimportRequested_{false};
  std::atomic<bool> clearRequested_{false};
};
} // namespace net::minecraft::client::render::terrain
