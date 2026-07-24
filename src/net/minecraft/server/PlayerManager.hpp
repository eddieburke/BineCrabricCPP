#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/server/ChunkMap.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/server/world/PlayerSaveHandler.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
namespace net::minecraft {
class Packet;
namespace block::entity {
class BlockEntity;
}
namespace entity::player {
class PlayerEntity;
class ServerPlayerEntity;
} // namespace entity::player
namespace server {
class MinecraftServer;
namespace network {
class ServerLoginNetworkHandler;
}
class PlayerManager {
 public:
 explicit PlayerManager(MinecraftServer* server);
 void configureFromProperties();
 void saveAllPlayers(ServerWorld* worlds[2]);
 void updatePlayerAfterDimensionChange(::net::minecraft::entity::player::ServerPlayerEntity* player);
 [[nodiscard]] int getBlockViewDistance() const;
 void loadPlayerData(::net::minecraft::entity::player::ServerPlayerEntity* player);
 void addPlayer(::net::minecraft::entity::player::ServerPlayerEntity* player);
 void updatePlayerChunks(::net::minecraft::entity::player::ServerPlayerEntity* player);
 void sendPendingChunks(::net::minecraft::entity::player::ServerPlayerEntity* player);
 void disconnect(::net::minecraft::entity::player::ServerPlayerEntity* player);
 ::net::minecraft::entity::player::ServerPlayerEntity* connectPlayer(
     network::ServerLoginNetworkHandler* loginNetworkHandler, const std::string& name);
 ::net::minecraft::entity::player::ServerPlayerEntity* respawnPlayer(
     ::net::minecraft::entity::player::ServerPlayerEntity* player, int dimensionId);
 void changePlayerDimension(::net::minecraft::entity::player::ServerPlayerEntity* player);
 void updateAllChunks();
 void markDirty(int x, int y, int z, int dimensionId);
 template <typename PacketT>
 void sendToAll(const PacketT& packet) {
  for(::net::minecraft::entity::player::ServerPlayerEntity* player : players) {
   if(player != nullptr && player->networkHandler != nullptr) {
    player->networkHandler->sendPacket(packet);
   }
  }
 }
 template <typename PacketT>
 void sendToDimension(const PacketT& packet, int dimensionId) {
  for(::net::minecraft::entity::player::ServerPlayerEntity* player : players) {
   if(player != nullptr && player->dimensionId == dimensionId && player->networkHandler != nullptr) {
    player->networkHandler->sendPacket(packet);
   }
  }
 }
 template <typename PacketT>
 void sendToAround(double x, double y, double z, double range, int dimensionId, const PacketT& packet) {
  sendToAround(nullptr, x, y, z, range, dimensionId, packet);
 }
 template <typename PacketT>
 void sendToAround(::net::minecraft::entity::player::PlayerEntity* player,
                   double x,
                   double y,
                   double z,
                   double range,
                   int dimensionId,
                   const PacketT& packet) {
  const double rangeSq = range * range;
  for(::net::minecraft::entity::player::ServerPlayerEntity* serverPlayer : players) {
   if(serverPlayer == nullptr ||
      static_cast<::net::minecraft::entity::player::PlayerEntity*>(serverPlayer) == player ||
      serverPlayer->dimensionId != dimensionId || serverPlayer->networkHandler == nullptr) {
    continue;
   }
   const double deltaX = x - serverPlayer->x;
   const double deltaY = y - serverPlayer->y;
   const double deltaZ = z - serverPlayer->z;
   if(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ >= rangeSq) {
    continue;
   }
   serverPlayer->networkHandler->sendPacket(packet);
  }
 }
 [[nodiscard]] std::string getPlayerList() const;
 void banPlayer(const std::string& name);
 void unbanPlayer(const std::string& name);
 void banIp(const std::string& ip);
 void unbanIp(const std::string& ip);
 void addToOperators(const std::string& name);
 void addTransientOperator(const std::string& name);
 void removeFromOperators(const std::string& name);
 [[nodiscard]] bool isWhitelisted(const std::string& name) const;
 [[nodiscard]] bool isOperator(const std::string& name) const;
 [[nodiscard]] bool hasOperators() const {
  return !ops_.empty();
 }
 ::net::minecraft::entity::player::ServerPlayerEntity* getPlayer(const std::string& name);
 void messagePlayer(const std::string& name, const std::string& message);
 void broadcast(const std::string& message);
 template <typename PacketT>
 bool sendPacket(const std::string& playerName, const PacketT& packet) {
  if(::net::minecraft::entity::player::ServerPlayerEntity* player = getPlayer(playerName);
     player != nullptr && player->networkHandler != nullptr) {
   player->networkHandler->sendPacket(packet);
   return true;
  }
  return false;
 }
 void savePlayers();
 void updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity);
 void addToWhitelist(const std::string& name);
 void removeFromWhitelist(const std::string& name);
 [[nodiscard]] const std::unordered_set<std::string>& getWhitelist() const {
  return whitelist_;
 }
 void reloadWhitelist();
 void sendWorldInfo(::net::minecraft::entity::player::ServerPlayerEntity* player, ServerWorld* world);
 void sendPlayerStatus(::net::minecraft::entity::player::ServerPlayerEntity* player);
 std::vector<::net::minecraft::entity::player::ServerPlayerEntity*> players;

 private:
 [[nodiscard]] ChunkMap& getChunkMap(int dimensionId);
 void loadBannedPlayers();
 void saveBannedPlayers();
 void loadBannedIps();
 void saveBannedIps();
 void loadOperators();
 void saveOperators();
 void loadWhitelist();
 void saveWhitelist();
 MinecraftServer* server_ = nullptr;
 std::unique_ptr<ChunkMap> chunkMaps_[2]{};
 int maxPlayerCount_ = 20;
 std::unordered_set<std::string> bannedPlayers_;
 std::unordered_set<std::string> bannedIps_;
 std::unordered_set<std::string> ops_;
 std::unordered_set<std::string> whitelist_;
 std::filesystem::path bannedPlayersFile_;
 std::filesystem::path bannedIpsFile_;
 std::filesystem::path operatorsFile_;
 std::filesystem::path whitelistFile_;
 world::PlayerSaveHandler* saveHandler_ = nullptr;
 bool whitelistEnabled_ = false;
};
} // namespace server
} // namespace net::minecraft
