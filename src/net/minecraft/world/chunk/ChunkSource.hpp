#pragma once
#include <cstdint>
#include <memory>
#include <string>
namespace net::minecraft::client::gui::screen {
class LoadingDisplay;
}
namespace net::minecraft {
class Chunk;
class ChunkSource {
 public:
 virtual ~ChunkSource() = default;
 [[nodiscard]] virtual bool isChunkLoaded(int chunkX, int chunkZ) const = 0;
 [[nodiscard]] virtual Chunk& getChunk(int chunkX, int chunkZ) = 0;
 [[nodiscard]] virtual Chunk& loadChunk(int chunkX, int chunkZ) = 0;
 virtual void decorate(ChunkSource* source, int chunkX, int chunkZ) = 0;
 virtual bool save(bool saveEntities, client::gui::screen::LoadingDisplay* display) = 0;
 virtual bool tick() = 0;
 [[nodiscard]] virtual bool canSave() const = 0;
 [[nodiscard]] virtual std::string getDebugInfo() const = 0;
 virtual void setChunkCacheCenter(int /*chunkX*/, int /*chunkZ*/) {
 }
 virtual void setActiveRadius(int /*radius*/) {
 }
 virtual void pumpChunkPublish() {
 }
 virtual void populateReadyChunks() {
 }
 virtual void prefetchChunksNear(int /*centerChunkX*/, int /*centerChunkZ*/) {
 }
 virtual void prepareForSave() {
 }
};
} // namespace net::minecraft
