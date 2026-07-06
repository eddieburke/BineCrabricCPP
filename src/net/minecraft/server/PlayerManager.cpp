#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/network/packet/ChatPackets.hpp"
#include "net/minecraft/network/packet/WorldPackets.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/ServerLog.hpp"
#include "net/minecraft/server/ServerProperties.hpp"
#include "net/minecraft/server/network/ServerLoginNetworkHandler.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/server/network/ServerPlayerInteractionManager.hpp"
#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include "net/minecraft/world/dimension/PortalForcer.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
namespace net::minecraft::server {
namespace {
std::string toLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
std::string trimCopy(std::string value) {
  const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
  value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
  return value;
}
ChunkCache* chunkCacheFor(ServerWorld* world) {
  if(world == nullptr) {
    return nullptr;
  }
  return dynamic_cast<ChunkCache*>(world->getChunkSource());
}
} // namespace
PlayerManager::PlayerManager(MinecraftServer* server) : server_(server) {
  if(server_ != nullptr) {
    bannedPlayersFile_ = server_->getFile("banned-players.txt");
    bannedIpsFile_ = server_->getFile("banned-ips.txt");
    operatorsFile_ = server_->getFile("ops.txt");
    whitelistFile_ = server_->getFile("white-list.txt");
  }
  loadBannedPlayers();
  loadBannedIps();
  loadOperators();
  loadWhitelist();
  saveBannedPlayers();
  saveBannedIps();
  saveOperators();
  saveWhitelist();
}
void PlayerManager::configureFromProperties() {
  int viewDistance = 10;
  if(server_ != nullptr && server_->properties != nullptr) {
    viewDistance = server_->properties->getProperty("view-distance", 10);
    maxPlayerCount_ = server_->properties->getProperty("max-players", 20);
    whitelistEnabled_ = server_->properties->getProperty("white-list", false);
  }
  chunkMaps_[0] = std::make_unique<ChunkMap>(server_, 0, viewDistance);
  chunkMaps_[1] = std::make_unique<ChunkMap>(server_, -1, viewDistance);
}
void PlayerManager::saveAllPlayers(ServerWorld* worlds[2]) {
  if(worlds != nullptr && worlds[0] != nullptr && worlds[0]->getDimensionData() != nullptr) {
    saveHandler_ = worlds[0]->getDimensionData()->getPlayerSaveHandler();
  }
}
ChunkMap& PlayerManager::getChunkMap(int dimensionId) {
  return dimensionId == -1 ? *chunkMaps_[1] : *chunkMaps_[0];
}
int PlayerManager::getBlockViewDistance() const {
  return chunkMaps_[0] != nullptr ? chunkMaps_[0]->getBlockViewDistance() : 144;
}
void PlayerManager::loadPlayerData(::net::minecraft::entity::player::ServerPlayerEntity* player) {
  if(saveHandler_ != nullptr && player != nullptr) {
    saveHandler_->loadPlayerData(*player);
  }
}
void PlayerManager::addPlayer(::net::minecraft::entity::player::ServerPlayerEntity* player) {
  if(player == nullptr || server_ == nullptr) {
    return;
  }
  players.push_back(player);
  ServerWorld* serverWorld = server_->getWorld(player->dimensionId);
  if(serverWorld == nullptr) {
    return;
  }
  if(ChunkCache* cache = chunkCacheFor(serverWorld); cache != nullptr) {
    cache->loadChunk(static_cast<int>(player->x) >> 4, static_cast<int>(player->z) >> 4);
  }
  while(!serverWorld->getEntityCollisions(player, player->boundingBox).empty()) {
    player->setPosition(player->x, player->y + 1.0, player->z);
  }
  serverWorld->spawnEntity(player);
  getChunkMap(player->dimensionId).addPlayer(player);
}
void PlayerManager::updatePlayerChunks(::net::minecraft::entity::player::ServerPlayerEntity* player) {
  if(player != nullptr) {
    getChunkMap(player->dimensionId).updatePlayerChunks(player);
  }
}
void PlayerManager::disconnect(::net::minecraft::entity::player::ServerPlayerEntity* player) {
  if(player == nullptr || server_ == nullptr) {
    return;
  }
  if(saveHandler_ != nullptr) {
    saveHandler_->savePlayerData(*player);
  }
  if(ServerWorld* world = server_->getWorld(player->dimensionId); world != nullptr) {
    world->remove(player);
  }
  players.erase(std::remove(players.begin(), players.end(), player), players.end());
  getChunkMap(player->dimensionId).removePlayer(player);
}
::net::minecraft::entity::player::ServerPlayerEntity* PlayerManager::connectPlayer(
    network::ServerLoginNetworkHandler* loginNetworkHandler, const std::string& name) {
  if(loginNetworkHandler == nullptr || server_ == nullptr) {
    return nullptr;
  }
  const std::string trimmedName = trimCopy(name);
  if(bannedPlayers_.contains(toLowerCopy(trimmedName))) {
    loginNetworkHandler->disconnect("You are banned from this server!");
    return nullptr;
  }
  if(!isWhitelisted(trimmedName)) {
    loginNetworkHandler->disconnect("You are not white-listed on this server!");
    return nullptr;
  }
  std::string address = loginNetworkHandler->connection() != nullptr ? loginNetworkHandler->connection()->getAddress()
                                                                     : std::string{};
  const std::size_t slash = address.find('/');
  if(slash != std::string::npos) {
    address = address.substr(slash + 1);
  }
  const std::size_t colon = address.find(':');
  if(colon != std::string::npos) {
    address = address.substr(0, colon);
  }
  if(bannedIps_.contains(toLowerCopy(address))) {
    loginNetworkHandler->disconnect("Your IP address is banned from this server!");
    return nullptr;
  }
  if(static_cast<int>(players.size()) >= maxPlayerCount_) {
    loginNetworkHandler->disconnect("The server is full!");
    return nullptr;
  }
  for(::net::minecraft::entity::player::ServerPlayerEntity* existing : players) {
    if(existing != nullptr && existing->name.size() == trimmedName.size() &&
       std::equal(existing->name.begin(), existing->name.end(), trimmedName.begin(), trimmedName.end(),
                  [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); })) {
      if(existing->networkHandler != nullptr) {
        existing->networkHandler->disconnect("You logged in from another location");
      }
    }
  }
  ServerWorld* overworld = server_->getWorld(0);
  if(overworld == nullptr) {
    return nullptr;
  }
  network::ServerPlayerInteractionManager interactionManager(overworld);
  return new ::net::minecraft::entity::player::ServerPlayerEntity(server_, overworld, trimmedName, &interactionManager);
}
::net::minecraft::entity::player::ServerPlayerEntity* PlayerManager::respawnPlayer(
    ::net::minecraft::entity::player::ServerPlayerEntity* player, int dimensionId) {
  if(player == nullptr || server_ == nullptr) {
    return nullptr;
  }
  server_->getEntityTracker(player->dimensionId).removeListener(player);
  server_->getEntityTracker(player->dimensionId).onEntityRemoved(player);
  getChunkMap(player->dimensionId).removePlayer(player);
  players.erase(std::remove(players.begin(), players.end(), player), players.end());
  if(ServerWorld* oldWorld = server_->getWorld(player->dimensionId); oldWorld != nullptr) {
    oldWorld->serverRemove(player);
  }
  const std::optional<Vec3i> spawnPos = player->getSpawnPos();
  player->dimensionId = dimensionId;
  ServerWorld* targetWorld = server_->getWorld(player->dimensionId);
  if(targetWorld == nullptr) {
    return nullptr;
  }
  network::ServerPlayerInteractionManager interactionManager(targetWorld);
  auto* respawned =
      new ::net::minecraft::entity::player::ServerPlayerEntity(server_, targetWorld, player->name, &interactionManager);
  respawned->id = player->id;
  respawned->networkHandler = player->networkHandler;
  if(spawnPos.has_value()) {
    if(const std::optional<Vec3i> validSpawn =
           ::net::minecraft::entity::player::PlayerEntity::findRespawnPosition(targetWorld, *spawnPos);
       validSpawn.has_value()) {
      respawned->setPositionAndAnglesKeepPrevAngles(static_cast<float>(validSpawn->x) + 0.5f,
                                                    static_cast<float>(validSpawn->y) + 0.1f,
                                                    static_cast<float>(validSpawn->z) + 0.5f, 0.0f, 0.0f);
      respawned->setSpawnPos(spawnPos);
    } else if(respawned->networkHandler != nullptr) {
      GameStateChangeS2CPacket packet;
      packet.reason = 0;
      respawned->networkHandler->sendPacket(packet);
    }
  }
  if(ChunkCache* cache = chunkCacheFor(targetWorld); cache != nullptr) {
    cache->loadChunk(static_cast<int>(respawned->x) >> 4, static_cast<int>(respawned->z) >> 4);
  }
  while(!targetWorld->getEntityCollisions(respawned, respawned->boundingBox).empty()) {
    respawned->setPosition(respawned->x, respawned->y + 1.0, respawned->z);
  }
  if(respawned->networkHandler != nullptr) {
    PlayerRespawnPacket packet;
    packet.dimensionRawId = static_cast<std::int8_t>(respawned->dimensionId);
    respawned->networkHandler->sendPacket(packet);
    respawned->networkHandler->teleport(respawned->x, respawned->y, respawned->z, respawned->yaw, respawned->pitch);
  }
  sendWorldInfo(respawned, targetWorld);
  getChunkMap(respawned->dimensionId).addPlayer(respawned);
  targetWorld->spawnEntity(respawned);
  players.push_back(respawned);
  respawned->initScreenHandler();
  respawned->clearWakePosition();
  return respawned;
}
void PlayerManager::changePlayerDimension(::net::minecraft::entity::player::ServerPlayerEntity* player) {
  if(player == nullptr || server_ == nullptr) {
    return;
  }
  ServerWorld* oldWorld = server_->getWorld(player->dimensionId);
  const int targetDimension = player->dimensionId == -1 ? 0 : -1;
  player->dimensionId = targetDimension;
  ServerWorld* newWorld = server_->getWorld(player->dimensionId);
  if(oldWorld == nullptr || newWorld == nullptr) {
    return;
  }
  if(player->networkHandler != nullptr) {
    PlayerRespawnPacket packet;
    packet.dimensionRawId = static_cast<std::int8_t>(player->dimensionId);
    player->networkHandler->sendPacket(packet);
  }
  oldWorld->serverRemove(player);
  player->dead = false;
  double x = player->x;
  double z = player->z;
  constexpr double scale = 8.0;
  if(player->dimensionId == -1) {
    player->setPositionAndAnglesKeepPrevAngles(x / scale, player->y, z / scale, player->yaw, player->pitch);
    if(player->isAlive()) {
      oldWorld->updateEntity(player, false);
    }
  } else {
    player->setPositionAndAnglesKeepPrevAngles(x * scale, player->y, z * scale, player->yaw, player->pitch);
    if(player->isAlive()) {
      oldWorld->updateEntity(player, false);
    }
  }
  if(player->isAlive()) {
    newWorld->spawnEntity(player);
    player->setPositionAndAnglesKeepPrevAngles(x, player->y, z, player->yaw, player->pitch);
    newWorld->updateEntity(player, false);
    if(ChunkCache* cache = chunkCacheFor(newWorld); cache != nullptr) {
      cache->forceLoad = true;
      PortalForcer forcer;
      forcer.moveToPortal(newWorld, player);
      cache->forceLoad = false;
    }
  }
  updatePlayerAfterDimensionChange(player);
  if(player->networkHandler != nullptr) {
    player->networkHandler->teleport(player->x, player->y, player->z, player->yaw, player->pitch);
  }
  player->setWorld(newWorld);
  sendWorldInfo(player, newWorld);
  sendPlayerStatus(player);
}
void PlayerManager::updatePlayerAfterDimensionChange(::net::minecraft::entity::player::ServerPlayerEntity* player) {
  if(player == nullptr || server_ == nullptr) {
    return;
  }
  chunkMaps_[0]->removePlayer(player);
  chunkMaps_[1]->removePlayer(player);
  getChunkMap(player->dimensionId).addPlayer(player);
  if(ServerWorld* world = server_->getWorld(player->dimensionId); world != nullptr) {
    if(ChunkCache* cache = chunkCacheFor(world); cache != nullptr) {
      cache->loadChunk(static_cast<int>(player->x) >> 4, static_cast<int>(player->z) >> 4);
    }
  }
}
void PlayerManager::updateAllChunks() {
  for(const std::unique_ptr<ChunkMap>& chunkMap : chunkMaps_) {
    if(chunkMap != nullptr) {
      chunkMap->updateChunks();
    }
  }
}
void PlayerManager::markDirty(int x, int y, int z, int dimensionId) {
  getChunkMap(dimensionId).markBlockForUpdate(x, y, z);
}
std::string PlayerManager::getPlayerList() const {
  std::string list;
  for(std::size_t i = 0; i < players.size(); ++i) {
    if(players[i] == nullptr) {
      continue;
    }
    if(!list.empty()) {
      list += ", ";
    }
    list += players[i]->name;
  }
  return list;
}
void PlayerManager::banPlayer(const std::string& name) {
  bannedPlayers_.insert(toLowerCopy(name));
  saveBannedPlayers();
}
void PlayerManager::unbanPlayer(const std::string& name) {
  bannedPlayers_.erase(toLowerCopy(name));
  saveBannedPlayers();
}
void PlayerManager::loadBannedPlayers() {
  try {
    bannedPlayers_.clear();
    std::ifstream input(bannedPlayersFile_);
    std::string line;
    while(std::getline(input, line)) {
      bannedPlayers_.insert(toLowerCopy(trimCopy(line)));
    }
  } catch(const std::exception& exception) {
    ServerLog::LOGGER.log(LogLevel::Warning, "Failed to load ban list: " + std::string(exception.what()));
  }
}
void PlayerManager::saveBannedPlayers() {
  try {
    std::ofstream output(bannedPlayersFile_, std::ios::trunc);
    for(const std::string& name : bannedPlayers_) {
      output << name << '\n';
    }
  } catch(const std::exception& exception) {
    ServerLog::LOGGER.log(LogLevel::Warning, "Failed to save ban list: " + std::string(exception.what()));
  }
}
void PlayerManager::banIp(const std::string& ip) {
  bannedIps_.insert(toLowerCopy(ip));
  saveBannedIps();
}
void PlayerManager::unbanIp(const std::string& ip) {
  bannedIps_.erase(toLowerCopy(ip));
  saveBannedIps();
}
void PlayerManager::loadBannedIps() {
  try {
    bannedIps_.clear();
    std::ifstream input(bannedIpsFile_);
    std::string line;
    while(std::getline(input, line)) {
      bannedIps_.insert(toLowerCopy(trimCopy(line)));
    }
  } catch(const std::exception& exception) {
    ServerLog::LOGGER.log(LogLevel::Warning, "Failed to load ip ban list: " + std::string(exception.what()));
  }
}
void PlayerManager::saveBannedIps() {
  try {
    std::ofstream output(bannedIpsFile_, std::ios::trunc);
    for(const std::string& ip : bannedIps_) {
      output << ip << '\n';
    }
  } catch(const std::exception& exception) {
    ServerLog::LOGGER.log(LogLevel::Warning, "Failed to save ip ban list: " + std::string(exception.what()));
  }
}
void PlayerManager::addToOperators(const std::string& name) {
  ops_.insert(toLowerCopy(name));
  saveOperators();
}
void PlayerManager::removeFromOperators(const std::string& name) {
  ops_.erase(toLowerCopy(name));
  saveOperators();
}
void PlayerManager::loadOperators() {
  try {
    ops_.clear();
    std::ifstream input(operatorsFile_);
    std::string line;
    while(std::getline(input, line)) {
      ops_.insert(toLowerCopy(trimCopy(line)));
    }
  } catch(const std::exception& exception) {
    ServerLog::LOGGER.log(LogLevel::Warning, "Failed to load ip ban list: " + std::string(exception.what()));
  }
}
void PlayerManager::saveOperators() {
  try {
    std::ofstream output(operatorsFile_, std::ios::trunc);
    for(const std::string& name : ops_) {
      output << name << '\n';
    }
  } catch(const std::exception& exception) {
    ServerLog::LOGGER.log(LogLevel::Warning, "Failed to save ip ban list: " + std::string(exception.what()));
  }
}
void PlayerManager::loadWhitelist() {
  try {
    whitelist_.clear();
    std::ifstream input(whitelistFile_);
    std::string line;
    while(std::getline(input, line)) {
      whitelist_.insert(toLowerCopy(trimCopy(line)));
    }
  } catch(const std::exception& exception) {
    ServerLog::LOGGER.log(LogLevel::Warning, "Failed to load white-list: " + std::string(exception.what()));
  }
}
void PlayerManager::saveWhitelist() {
  try {
    std::ofstream output(whitelistFile_, std::ios::trunc);
    for(const std::string& name : whitelist_) {
      output << name << '\n';
    }
  } catch(const std::exception& exception) {
    ServerLog::LOGGER.log(LogLevel::Warning, "Failed to save white-list: " + std::string(exception.what()));
  }
}
bool PlayerManager::isWhitelisted(const std::string& name) const {
  const std::string lowered = toLowerCopy(trimCopy(name));
  return !whitelistEnabled_ || ops_.contains(lowered) || whitelist_.contains(lowered);
}
bool PlayerManager::isOperator(const std::string& name) const {
  return ops_.contains(toLowerCopy(trimCopy(name)));
}
::net::minecraft::entity::player::ServerPlayerEntity* PlayerManager::getPlayer(const std::string& name) {
  for(::net::minecraft::entity::player::ServerPlayerEntity* player : players) {
    if(player != nullptr && player->name.size() == name.size() &&
       std::equal(player->name.begin(), player->name.end(), name.begin(), name.end(),
                  [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); })) {
      return player;
    }
  }
  return nullptr;
}
void PlayerManager::messagePlayer(const std::string& name, const std::string& message) {
  if(::net::minecraft::entity::player::ServerPlayerEntity* player = getPlayer(name);
     player != nullptr && player->networkHandler != nullptr) {
    ChatMessagePacket packet;
    packet.chatMessage = message;
    player->networkHandler->sendPacket(packet);
  }
}
void PlayerManager::broadcast(const std::string& message) {
  ChatMessagePacket packet;
  packet.chatMessage = message;
  for(::net::minecraft::entity::player::ServerPlayerEntity* player : players) {
    if(player != nullptr && isOperator(player->name) && player->networkHandler != nullptr) {
      player->networkHandler->sendPacket(packet);
    }
  }
}
void PlayerManager::savePlayers() {
  if(saveHandler_ == nullptr) {
    return;
  }
  for(::net::minecraft::entity::player::ServerPlayerEntity* player : players) {
    if(player != nullptr) {
      saveHandler_->savePlayerData(*player);
    }
  }
}
void PlayerManager::updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity) {
  (void)x;
  (void)y;
  (void)z;
  (void)blockEntity;
}
void PlayerManager::addToWhitelist(const std::string& name) {
  whitelist_.insert(name);
  saveWhitelist();
}
void PlayerManager::removeFromWhitelist(const std::string& name) {
  whitelist_.erase(name);
  saveWhitelist();
}
void PlayerManager::reloadWhitelist() {
  loadWhitelist();
}
void PlayerManager::sendWorldInfo(::net::minecraft::entity::player::ServerPlayerEntity* player, ServerWorld* world) {
  if(player == nullptr || world == nullptr || player->networkHandler == nullptr) {
    return;
  }
  WorldTimeUpdateS2CPacket timePacket;
  timePacket.time = static_cast<std::int64_t>(world->getTime());
  player->networkHandler->sendPacket(timePacket);
  if(world->isRaining()) {
    GameStateChangeS2CPacket rainPacket;
    rainPacket.reason = 1;
    player->networkHandler->sendPacket(rainPacket);
  }
}
void PlayerManager::sendPlayerStatus(::net::minecraft::entity::player::ServerPlayerEntity* player) {
  if(player != nullptr) {
    player->onContentsUpdate(player->playerScreenHandler);
    player->markHealthDirty();
  }
}
} // namespace net::minecraft::server
