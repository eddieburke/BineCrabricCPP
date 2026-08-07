#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
namespace net::minecraft {
class Chunk;
class ChunkSource {
 public:
 // Progress callback for save operations: receives [0,100] percentage.
 // Default no-op ignores progress.
 using SaveProgressCallback = std::function<void(int percent)>;
 virtual ~ChunkSource() = default;
 [[nodiscard]] virtual bool isChunkLoaded(int chunkX, int chunkZ) const = 0;
 [[nodiscard]] virtual bool isChunkDataReady(int chunkX, int chunkZ) const {
  return isChunkLoaded(chunkX, chunkZ);
 }
 virtual void markChunkDataReady(int /*chunkX*/, int /*chunkZ*/) {
 }
 [[nodiscard]] virtual Chunk& getChunk(int chunkX, int chunkZ) = 0;
 [[nodiscard]] virtual Chunk& loadChunk(int chunkX, int chunkZ) = 0;
 virtual void decorate(ChunkSource* source, int chunkX, int chunkZ) = 0;
 virtual bool save(bool saveEntities, SaveProgressCallback progress = nullptr) = 0;
 virtual bool tick() = 0;
 [[nodiscard]] virtual bool canSave() const = 0;
 [[nodiscard]] virtual std::string getDebugInfo() const = 0;
 virtual void setChunkCacheCenter(int /*chunkX*/, int /*chunkZ*/) {
 }
 virtual void setActiveRadius(int /*radius*/) {
 }
 virtual void pumpChunkPublish() {
 }
 virtual void prefetchChunksNear(int /*centerChunkX*/, int /*centerChunkZ*/) {
 }
 virtual void prepareForSave() {
 }
};
} // namespace net::minecraft
