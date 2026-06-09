#include "net/minecraft/server/PlayerManager.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"

#include <algorithm>
#include <cctype>

namespace net::minecraft::server {

namespace {

std::string toLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

} // namespace

PlayerManager::PlayerManager(MinecraftServer* server)
    : server_(server)
{
    (void)server_;
}

void PlayerManager::updateAllChunks()
{
}

void PlayerManager::markDirty(int x, int y, int z, int dimensionId)
{
    (void)x;
    (void)y;
    (void)z;
    (void)dimensionId;
}

void PlayerManager::changePlayerDimension(entity::player::ServerPlayerEntity* player)
{
    (void)player;
}

void PlayerManager::updatePlayerChunks(entity::player::ServerPlayerEntity* player)
{
    (void)player;
}

void PlayerManager::sendToAll()
{
}

void PlayerManager::sendToAround(
    entity::player::PlayerEntity* player,
    double x,
    double y,
    double z,
    double range,
    int dimensionId)
{
    (void)player;
    (void)x;
    (void)y;
    (void)z;
    (void)range;
    (void)dimensionId;
}

bool PlayerManager::isOperator(const std::string& name) const
{
    return ops_.contains(toLowerCopy(name));
}

} // namespace net::minecraft::server
