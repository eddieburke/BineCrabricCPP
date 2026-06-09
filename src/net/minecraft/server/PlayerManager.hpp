#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace net::minecraft::entity::player {
class PlayerEntity;
class ServerPlayerEntity;
}

namespace net::minecraft::server {

class MinecraftServer;

// Player list, ops, and chunk/block broadcast routing (packets stubbed until network layer).
class PlayerManager {
public:
    explicit PlayerManager(MinecraftServer* server);

    void updateAllChunks();
    void markDirty(int x, int y, int z, int dimensionId);
    void changePlayerDimension(entity::player::ServerPlayerEntity* player);
    void updatePlayerChunks(entity::player::ServerPlayerEntity* player);

    void sendToAll();
    void sendToAround(
        entity::player::PlayerEntity* player,
        double x,
        double y,
        double z,
        double range,
        int dimensionId);
    void sendToAround(double x, double y, double z, double range, int dimensionId)
    {
        sendToAround(nullptr, x, y, z, range, dimensionId);
    }

    [[nodiscard]] int getBlockViewDistance() const { return blockViewDistance_; }
    [[nodiscard]] bool isOperator(const std::string& name) const;

    std::vector<entity::player::ServerPlayerEntity*> players;

private:
    MinecraftServer* server_ = nullptr;
    int blockViewDistance_ = 144;
    std::unordered_set<std::string> ops_;
};

} // namespace net::minecraft::server
