#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/gui/screen/LoadingDisplay.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
namespace net::minecraft {
class Chunk;
class ServerWorld;
namespace server::world::chunk {
class ServerChunkCache : public ChunkSource {
public:
  ServerChunkCache(ServerWorld* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator);
  bool forceLoad = false;
  [[nodiscard]] bool isChunkLoaded(int chunkX, int chunkZ) const override;
  void isLoaded(int chunkX, int chunkZ);
  Chunk& getChunk(int chunkX, int chunkZ) override;
  Chunk& loadChunk(int chunkX, int chunkZ) override;
  void decorate(ChunkSource* source, int chunkX, int chunkZ) override;
  bool save(bool saveEntityData, client::gui::screen::LoadingDisplay* display) override;
  bool tick() override;
  [[nodiscard]] bool canSave() const override;
  [[nodiscard]] std::string getDebugInfo() const override;

private:
  Chunk* loadChunkFromStorage(int chunkX, int chunkZ);
  void saveEntities(Chunk& chunk);
  void saveChunk(Chunk& chunk);
  EmptyChunk empty_;
  ServerWorld* world_ = nullptr;
  std::unique_ptr<ChunkStorage> storage_{};
  ChunkSource* generator_ = nullptr;
  std::unordered_map<ChunkPos, Chunk*, ChunkPosHash> chunksByPos_{};
  std::vector<Chunk*> chunks_{};
  std::vector<std::unique_ptr<Chunk>> ownedChunks_{};
  std::unordered_set<ChunkPos, ChunkPosHash> chunksToUnload_{};
};
} // namespace server::world::chunk
} // namespace net::minecraft
