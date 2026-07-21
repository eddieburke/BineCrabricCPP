#include "net/minecraft/server/ChunkMap.hpp"
#include <algorithm>
#include <climits>
#include <memory>
#include <stdexcept>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/packet/BlockPackets.hpp"
#include "net/minecraft/network/packet/ChunkPackets.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
namespace net::minecraft::server {
namespace {
using ::net::minecraft::BlockUpdateS2CPacket;
using ::net::minecraft::ChunkDataS2CPacket;
using ::net::minecraft::ChunkDeltaUpdateS2CPacket;
using ::net::minecraft::ChunkPos;
using ::net::minecraft::ChunkStatusUpdateS2CPacket;
using ::net::minecraft::Packet;
using ::net::minecraft::ServerWorld;
using ::net::minecraft::block::Block;
using ::net::minecraft::block::entity::BlockEntity;
using ::net::minecraft::entity::player::ServerPlayerEntity;
} // namespace
class ChunkMap::TrackedChunk : public std::enable_shared_from_this<ChunkMap::TrackedChunk> {
 public:
 TrackedChunk(ChunkMap* owner, int chunkX, int chunkZ)
     : owner_(owner), chunkX_(chunkX), chunkZ_(chunkZ), chunkPos_(chunkX, chunkZ) {
  if(ServerWorld* world = owner_->getWorld(); world != nullptr) {
   if(ChunkSource* chunkCache = world->getChunkSource(); chunkCache != nullptr) {
    chunkCache->loadChunk(chunkX, chunkZ);
   }
  }
 }
 void addPlayer(ServerPlayerEntity* player) {
  if(std::find(players_.begin(), players_.end(), player) != players_.end()) {
   throw std::runtime_error("Failed to add player. player already is in chunk " + std::to_string(chunkX_) +
                            ", " + std::to_string(chunkZ_));
  }
  player->activeChunks.insert(chunkPos_);
  if(player->networkHandler != nullptr) {
   ChunkStatusUpdateS2CPacket packet;
   packet.x = chunkPos_.x;
   packet.z = chunkPos_.z;
   packet.load = true;
   player->networkHandler->sendPacket(packet);
  }
  players_.push_back(player);
  player->pendingChunkUpdates.push_back(chunkPos_);
 }
 void removePlayer(ServerPlayerEntity* player) {
  if(std::find(players_.begin(), players_.end(), player) == players_.end()) {
   return;
  }
  players_.erase(std::remove(players_.begin(), players_.end(), player), players_.end());
  if(players_.empty()) {
   const std::int64_t key = ChunkMap::chunkKey(chunkX_, chunkZ_);
   owner_->chunkMapping_.remove(key);
   if(dirtyBlockCount_ > 0) {
    const auto end =
        std::remove_if(owner_->chunksToUpdate_.begin(),
                       owner_->chunksToUpdate_.end(),
                       [&](const std::shared_ptr<TrackedChunk>& tracked) { return tracked.get() == this; });
    owner_->chunksToUpdate_.erase(end, owner_->chunksToUpdate_.end());
   }
   if(ServerWorld* world = owner_->getWorld(); world != nullptr) {
    if(auto* chunkCache = dynamic_cast<net::minecraft::world::chunk::ChunkCache*>(world->getChunkSource());
       chunkCache != nullptr) {
     chunkCache->dropChunk(chunkX_, chunkZ_);
    }
   }
  }
  const auto pendingIt =
      std::find(player->pendingChunkUpdates.begin(), player->pendingChunkUpdates.end(), chunkPos_);
  if(pendingIt != player->pendingChunkUpdates.end()) {
   player->pendingChunkUpdates.erase(pendingIt);
  }
  const bool wasActive = player->activeChunks.erase(chunkPos_) > 0;
  if(wasActive && player->networkHandler != nullptr) {
   ChunkStatusUpdateS2CPacket packet;
   packet.x = chunkX_;
   packet.z = chunkZ_;
   packet.load = false;
   player->networkHandler->sendPacket(packet);
  }
 }
  void updatePlayerChunks(int x, int y, int z) {
   if(dirtyBlockCount_ == 0) {
    owner_->chunksToUpdate_.push_back(shared_from_this());
    minX_ = x;
    minY_ = y;
    minZ_ = z;
    maxX_ = x;
    maxY_ = y;
    maxZ_ = z;
   }
   if(minX_ > x) {
    minX_ = x;
   }
   if(maxX_ < x) {
    maxX_ = x;
   }
   if(minY_ > y) {
    minY_ = y;
   }
   if(maxY_ < y) {
    maxY_ = y;
   }
   if(minZ_ > z) {
    minZ_ = z;
   }
   if(maxZ_ < z) {
    maxZ_ = z;
   }
   if(dirtyBlockCount_ < 10) {
    const std::int16_t encoded = static_cast<std::int16_t>((x << 12) | (z << 8) | y);
    for(int i = 0; i < dirtyBlockCount_; ++i) {
     if(dirtyBlocks_[static_cast<std::size_t>(i)] == encoded) {
      return;
     }
    }
    dirtyBlocks_[static_cast<std::size_t>(dirtyBlockCount_++)] = encoded;
   }
  }
  template <typename PacketT>
  void sendPacketToPlayers(const PacketT& packet) {
   for(ServerPlayerEntity* player : players_) {
    if(player == nullptr || player->networkHandler == nullptr) {
     continue;
    }
    if(!player->activeChunks.contains(chunkPos_)) {
     continue;
    }
    player->networkHandler->sendPacket(packet);
   }
  }
  void updateChunk() {
   ServerWorld* serverWorld = owner_->getWorld();
   if(serverWorld == nullptr || dirtyBlockCount_ == 0) {
    return;
   }
   if(dirtyBlockCount_ == 1) {
    const int blockX = chunkX_ * 16 + minX_;
    const int blockY = minY_;
    const int blockZ = chunkZ_ * 16 + minZ_;
   BlockUpdateS2CPacket packet;
   packet.x = blockX;
   packet.y = blockY;
   packet.z = blockZ;
   packet.blockRawId = serverWorld->getBlockId(blockX, blockY, blockZ);
   packet.blockMetadata = serverWorld->getBlockMeta(blockX, blockY, blockZ);
   sendPacketToPlayers(packet);
   if(Block::BLOCKS_WITH_ENTITY[static_cast<std::size_t>(serverWorld->getBlockId(blockX, blockY, blockZ))]) {
    sendBlockEntityUpdate(serverWorld->getBlockEntity(blockX, blockY, blockZ));
   }
  } else if(dirtyBlockCount_ == 10) {
   minY_ = minY_ / 2 * 2;
   maxY_ = (maxY_ / 2 + 1) * 2;
   const int blockX = minX_ + chunkX_ * 16;
   const int blockY = minY_;
   const int blockZ = minZ_ + chunkZ_ * 16;
   const int sizeX = maxX_ - minX_ + 1;
   const int sizeY = maxY_ - minY_ + 2;
   const int sizeZ = maxZ_ - minZ_ + 1;
   ChunkDataS2CPacket packet;
   packet.x = blockX;
   packet.y = blockY;
   packet.z = blockZ;
   packet.sizeX = sizeX;
   packet.sizeY = sizeY;
   packet.sizeZ = sizeZ;
   packet.chunkData = serverWorld->getChunkData(blockX, blockY, blockZ, sizeX, sizeY, sizeZ);
   packet.compressForSend();
   sendPacketToPlayers(packet);
   const std::vector<BlockEntity*> blockEntities =
       serverWorld->getBlockEntities(blockX, blockY, blockZ, blockX + sizeX, blockY + sizeY, blockZ + sizeZ);
   for(BlockEntity* blockEntity : blockEntities) {
    sendBlockEntityUpdate(blockEntity);
   }
  } else {
   ChunkDeltaUpdateS2CPacket packet;
   packet.x = chunkX_;
   packet.z = chunkZ_;
   packet.entryCount = dirtyBlockCount_;
   packet.positions.resize(static_cast<std::size_t>(dirtyBlockCount_));
   packet.blockRawIds.resize(static_cast<std::size_t>(dirtyBlockCount_));
   packet.blockMetadata.resize(static_cast<std::size_t>(dirtyBlockCount_));
   Chunk& chunk = serverWorld->getChunk(chunkX_, chunkZ_);
   for(int i = 0; i < dirtyBlockCount_; ++i) {
    const std::int16_t position = dirtyBlocks_[static_cast<std::size_t>(i)];
    const int localX = (position >> 12) & 0xF;
    const int localZ = (position >> 8) & 0xF;
    const int localY = position & 0xFF;
    packet.positions[static_cast<std::size_t>(i)] = position;
    packet.blockRawIds[static_cast<std::size_t>(i)] =
        static_cast<std::int8_t>(chunk.getBlockId(localX, localY, localZ));
    packet.blockMetadata[static_cast<std::size_t>(i)] =
        static_cast<std::int8_t>(chunk.getBlockMeta(localX, localY, localZ));
   }
   sendPacketToPlayers(packet);
   for(int i = 0; i < dirtyBlockCount_; ++i) {
    const std::int16_t position = dirtyBlocks_[static_cast<std::size_t>(i)];
    const int blockX = chunkX_ * 16 + ((position >> 12) & 0xF);
    const int blockY = position & 0xFF;
    const int blockZ = chunkZ_ * 16 + ((position >> 8) & 0xF);
    if(!Block::BLOCKS_WITH_ENTITY[static_cast<std::size_t>(
           serverWorld->getBlockId(blockX, blockY, blockZ))]) {
     continue;
    }
    sendBlockEntityUpdate(serverWorld->getBlockEntity(blockX, blockY, blockZ));
   }
  }
  dirtyBlockCount_ = 0;
 }

 public:
 void sendBlockEntityUpdate(const BlockEntity* blockEntity) {
  if(blockEntity == nullptr) {
   return;
  }
  for(ServerPlayerEntity* player : players_) {
   if(player == nullptr || player->networkHandler == nullptr) {
    continue;
   }
   if(!player->networkHandler->modProtocolEnabled()) {
    continue;
   }
   if(!player->activeChunks.contains(chunkPos_)) {
    continue;
   }
   if(std::unique_ptr<Packet> packet = blockEntity->createUpdatePacket()) {
    player->networkHandler->sendPacket(std::move(packet));
   }
  }
 }

 private:
 ChunkMap* owner_ = nullptr;
 std::vector<ServerPlayerEntity*> players_;
 int chunkX_ = 0;
 int chunkZ_ = 0;
 ChunkPos chunkPos_;
 std::array<std::int16_t, 10> dirtyBlocks_{};
 int dirtyBlockCount_ = 0;
 int minX_ = 0;
 int minY_ = 0;
 int minZ_ = 0;
 int maxX_ = 0;
 int maxY_ = 0;
 int maxZ_ = 0;
};
ChunkMap::ChunkMap(MinecraftServer* server, int dimensionId, int viewRadius)
    : server_(server), dimensionId_(dimensionId) {
 if(viewRadius > 15) {
  throw std::invalid_argument("Too big view radius!");
 }
 if(viewRadius < 3) {
  throw std::invalid_argument("Too small view radius!");
 }
 viewDistance_ = viewRadius;
}
ServerWorld* ChunkMap::getWorld() const {
 return server_ != nullptr ? server_->getWorld(dimensionId_) : nullptr;
}
void ChunkMap::updateChunks() {
 for(const std::shared_ptr<TrackedChunk>& tracked : chunksToUpdate_) {
  tracked->updateChunk();
 }
 chunksToUpdate_.clear();
}
std::shared_ptr<ChunkMap::TrackedChunk> ChunkMap::getOrCreateChunk(int chunkX, int chunkZ, bool createIfAbsent) {
 const std::int64_t key = chunkKey(chunkX, chunkZ);
 std::shared_ptr<TrackedChunk> tracked = chunkMapping_.get(key);
 if(!tracked && createIfAbsent) {
  tracked = std::make_shared<TrackedChunk>(this, chunkX, chunkZ);
  chunkMapping_.put(key, tracked);
 }
 return tracked;
}
void ChunkMap::markBlockForUpdate(int x, int y, int z) {
 const int chunkX = x >> 4;
 const int chunkZ = z >> 4;
 if(std::shared_ptr<TrackedChunk> tracked = getOrCreateChunk(chunkX, chunkZ, false)) {
  tracked->updatePlayerChunks(x & 0xF, y, z & 0xF);
 }
}
void ChunkMap::sendBlockEntityUpdate(int x, int z, const BlockEntity& blockEntity) {
 if(std::shared_ptr<TrackedChunk> tracked = getOrCreateChunk(x >> 4, z >> 4, false)) {
  tracked->sendBlockEntityUpdate(&blockEntity);
 }
}
void ChunkMap::addPlayer(ServerPlayerEntity* player) {
 int directionIndex = 0;
 const int chunkX = static_cast<int>(player->x) >> 4;
 const int chunkZ = static_cast<int>(player->z) >> 4;
 player->lastX = player->x;
 player->lastZ = player->z;
 int stepX = 0;
 const int viewRadius = viewDistance_;
 int stepZ = 0;
 getOrCreateChunk(chunkX, chunkZ, true)->addPlayer(player);
 for(int ring = 1; ring <= viewRadius * 2; ++ring) {
  for(int side = 0; side < 2; ++side) {
   const std::array<int, 2>& direction = kDirections[static_cast<std::size_t>(directionIndex++ % 4)];
   for(int step = 0; step < ring; ++step) {
    getOrCreateChunk(chunkX + (stepX += direction[0]), chunkZ + (stepZ += direction[1]), true)
        ->addPlayer(player);
   }
  }
 }
 directionIndex %= 4;
 for(int edge = 0; edge < viewRadius * 2; ++edge) {
  getOrCreateChunk(chunkX + (stepX += kDirections[static_cast<std::size_t>(directionIndex)][0]),
                   chunkZ + (stepZ += kDirections[static_cast<std::size_t>(directionIndex)][1]),
                   true)
      ->addPlayer(player);
 }
 players.push_back(player);
}
void ChunkMap::removePlayer(ServerPlayerEntity* player) {
 const int chunkX = static_cast<int>(player->lastX) >> 4;
 const int chunkZ = static_cast<int>(player->lastZ) >> 4;
 for(int x = chunkX - viewDistance_; x <= chunkX + viewDistance_; ++x) {
  for(int z = chunkZ - viewDistance_; z <= chunkZ + viewDistance_; ++z) {
   if(std::shared_ptr<TrackedChunk> tracked = getOrCreateChunk(x, z, false)) {
    tracked->removePlayer(player);
   }
  }
 }
 players.erase(std::remove(players.begin(), players.end(), player), players.end());
}
bool ChunkMap::isWithinViewDistance(int chunkX, int chunkZ, int centerX, int centerZ) const {
 const int deltaX = chunkX - centerX;
 const int deltaZ = chunkZ - centerZ;
 if(deltaX < -viewDistance_ || deltaX > viewDistance_) {
  return false;
 }
 return deltaZ >= -viewDistance_ && deltaZ <= viewDistance_;
}
void ChunkMap::updatePlayerChunks(ServerPlayerEntity* player) {
 const int chunkX = static_cast<int>(player->x) >> 4;
 const int chunkZ = static_cast<int>(player->z) >> 4;
 const double deltaX = player->lastX - player->x;
 const double deltaZ = player->lastZ - player->z;
 const double distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
 if(distanceSquared < 64.0) {
  return;
 }
 const int lastChunkX = static_cast<int>(player->lastX) >> 4;
 const int lastChunkZ = static_cast<int>(player->lastZ) >> 4;
 const int deltaChunkX = chunkX - lastChunkX;
 const int deltaChunkZ = chunkZ - lastChunkZ;
 if(deltaChunkX == 0 && deltaChunkZ == 0) {
  return;
 }
 for(int x = chunkX - viewDistance_; x <= chunkX + viewDistance_; ++x) {
  for(int z = chunkZ - viewDistance_; z <= chunkZ + viewDistance_; ++z) {
   if(!isWithinViewDistance(x, z, lastChunkX, lastChunkZ)) {
    getOrCreateChunk(x, z, true)->addPlayer(player);
   }
   if(isWithinViewDistance(x - deltaChunkX, z - deltaChunkZ, chunkX, chunkZ)) {
    continue;
   }
   if(std::shared_ptr<TrackedChunk> tracked = getOrCreateChunk(x - deltaChunkX, z - deltaChunkZ, false)) {
    tracked->removePlayer(player);
   }
  }
 }
 player->lastX = player->x;
 player->lastZ = player->z;
}
std::int64_t ChunkMap::chunkKey(int chunkX, int chunkZ) {
 const auto keyX = static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkX) + INT_MAX) & 0xffffffffULL;
 const auto keyZ = static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkZ) + INT_MAX) & 0xffffffffULL;
 return static_cast<std::int64_t>((keyZ << 32U) | keyX);
}
} // namespace net::minecraft::server
