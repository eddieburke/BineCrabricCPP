#pragma once
#include "net/minecraft/util/LongObjectHashMap.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <vector>
namespace net::minecraft {
class BlockUpdateS2CPacket;
class ChunkDataS2CPacket;
class ChunkDeltaUpdateS2CPacket;
class ChunkStatusUpdateS2CPacket;
class Packet;
class ServerWorld;
namespace block::entity {
class BlockEntity;
}
namespace entity::player {
class ServerPlayerEntity;
}
namespace server {
class MinecraftServer;
class ChunkMap {
public:
  std::vector<::net::minecraft::entity::player::ServerPlayerEntity*> players;
  ChunkMap(MinecraftServer* server, int dimensionId, int viewRadius);
  [[nodiscard]] ServerWorld* getWorld() const;
  void updateChunks();
  void markBlockForUpdate(int x, int y, int z);
  void addPlayer(::net::minecraft::entity::player::ServerPlayerEntity* player);
  void removePlayer(::net::minecraft::entity::player::ServerPlayerEntity* player);
  void updatePlayerChunks(::net::minecraft::entity::player::ServerPlayerEntity* player);
  [[nodiscard]] int getBlockViewDistance() const noexcept {
    return viewDistance_ * 16 - 16;
  }

private:
  class TrackedChunk;
  [[nodiscard]] std::shared_ptr<TrackedChunk> getOrCreateChunk(int chunkX, int chunkZ, bool createIfAbsent);
  [[nodiscard]] bool isWithinViewDistance(int chunkX, int chunkZ, int centerX, int centerZ) const;
  static std::int64_t chunkKey(int chunkX, int chunkZ);
  MinecraftServer* server_ = nullptr;
  util::LongObjectHashMap<std::shared_ptr<TrackedChunk>> chunkMapping_{};
  std::vector<std::shared_ptr<TrackedChunk>> chunksToUpdate_;
  int dimensionId_ = 0;
  int viewDistance_ = 0;
  static constexpr std::array<std::array<int, 2>, 4> kDirections{{{1, 0}, {0, 1}, {-1, 0}, {0, -1}}};
};
} // namespace server
} // namespace net::minecraft
