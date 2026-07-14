#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/chunk/ChunkRegionBuffer.hpp"
#include "net/minecraft/client/render/lod/LodData.hpp"
#include "net/minecraft/client/render/lod/LodMesher.hpp"
#include "net/minecraft/util/concurrent/WorkerHandoff.hpp"
namespace net::minecraft {
class World;
}
namespace net::minecraft::client::render::lod {
struct LodStats {
  bool enabled = false;
  bool importing = false;
  float distance = 0.0f;
  long long chunks = 0;
  long long regions = 0;
  long long meshedRegions = 0;
  long long pendingJobs = 0;
  long long drawnRegions = 0;
  long long importedChunks = 0;
};
class LodSystem {
public:
  static LodSystem& instance();
  LodSystem(const LodSystem&) = delete;
  LodSystem& operator=(const LodSystem&) = delete;
  void setWorld(net::minecraft::World* world);
  void onBlockChanged(int x, int y, int z);
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
  [[nodiscard]] bool activeForRender() const noexcept;
  [[nodiscard]] float lodDistanceBlocks() const noexcept {
    return distance_;
  }
  [[nodiscard]] bool fogExtendActive() const noexcept;
  void frameUpdate(double camX, double camY, double camZ, const option::ResolvedRenderOptions& resolved);
  void render(double camX, double camY, double camZ, float brightness, bool frustumCull);
  void requestReimport();
  void requestClear();
  [[nodiscard]] LodStats statsSnapshot() const;

private:
  LodSystem() = default;
  ~LodSystem();
  struct Region {
    long long dataStamp = 1;
    long long meshStamp = 0;
    int meshedLevel = -1;
    bool jobPending = false;
    bool hasMesh = false;
    int chunkCount = 0;
    float minY = 0.0f;
    float maxY = 0.0f;
    chunk::ChunkRegionBuffer::Slot slot{};
  };
  struct Group {
    chunk::ChunkRegionBuffer buffer{};
    double originX = 0.0;
    double originZ = 0.0;
    int meshCount = 0;
  };
  void clearAll();
  void stopImport();
  void startImport(double camX, double camZ);
  void importWorker(std::filesystem::path worldDir, int camChunkX, int camChunkZ);
  void drainImport();
  void liveScan(double camX, double camZ, int residentChunkRadius);
  void ingestChunk(const net::minecraft::Chunk& chunk);
  void mergeColumns(std::uint64_t key, const LodChunk& data, bool fromImport);
  void markRegionDirty(int regionX, int regionZ);
  void scheduleMeshes(double camX, double camZ);
  void drainMeshes();
  void evict(double camX, double camZ);
  void updateStats();
  [[nodiscard]] int levelForDistance(float blocks) const noexcept;
  net::minecraft::World* world_ = nullptr;
  bool worldIsNether_ = false;
  std::filesystem::path worldDir_{};
  bool enabled_ = true;
  float distance_ = 2048.0f;
  int detail_ = 0;
  bool fogExtend_ = true;
  bool importWorld_ = true;
  float nearCutoffBlocks_ = 0.0f;
  std::unordered_map<std::uint64_t, LodChunk> chunks_{};
  std::unordered_map<std::uint64_t, std::uint64_t> liveIngested_{};
  std::unordered_map<std::uint64_t, Region> regions_{};
  std::unordered_map<std::uint64_t, std::unique_ptr<Group>> groups_{};
  net::minecraft::util::concurrent::WorkerHandoff<LodMeshJob> meshWorkers_{};
  int jobsInFlight_ = 0;
  int scanCursor_ = 0;
  int evictCounter_ = 0;
  int drawnRegions_ = 0;
  std::mutex dirtyMutex_{};
  std::vector<std::uint64_t> dirtyChunks_{};
  std::thread importThread_{};
  std::atomic<bool> importRunning_{false};
  std::atomic<bool> importCancel_{false};
  std::atomic<long long> importedChunks_{0};
  bool importStartedForWorld_ = false;
  std::mutex importMutex_{};
  std::vector<std::pair<std::uint64_t, LodChunk>> importQueue_{};
  std::atomic<bool> reimportRequested_{false};
  std::atomic<bool> clearRequested_{false};
  unsigned brightnessTexture_ = 0;
  float lastBrightness_ = -1.0f;
  mutable std::mutex statsMutex_{};
  LodStats stats_{};
};
} // namespace net::minecraft::client::render::lod
