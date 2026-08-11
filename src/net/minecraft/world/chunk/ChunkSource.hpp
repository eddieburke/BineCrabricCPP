#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
namespace net::minecraft {
class Chunk;
class ChunkSource {
 public:
 using SaveProgressCallback = std::function<void(int percent)>;
 using LoadedChunkVisitor = std::function<void(int chunkX, int chunkZ, Chunk& chunk)>;
 virtual ~ChunkSource() = default;
 [[nodiscard]] virtual bool isChunkLoaded(int chunkX, int chunkZ) const = 0;
 [[nodiscard]] virtual bool isChunkDataReady(int chunkX, int chunkZ) const {
  return isChunkLoaded(chunkX, chunkZ);
 }
 [[nodiscard]] virtual Chunk* getChunkIfLoaded(int chunkX, int chunkZ) {
  return isChunkLoaded(chunkX, chunkZ) ? &getChunk(chunkX, chunkZ) : nullptr;
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
 virtual void forEachLoadedChunk(const LoadedChunkVisitor& visitor) {
  (void)visitor;
 }
};
} // namespace net::minecraft
