#pragma once

#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/server/entity/EntityTracker.hpp"

#include <array>
#include <string>

namespace net::minecraft {

class ServerWorld;

namespace server {

class MinecraftServer {
public:
    MinecraftServer();

    bool spawnAnimals = true;
    bool pvpEnabled = true;
    bool allowNether = true;
    bool flightEnabled = false;
    int ticks = 0;

    PlayerManager playerManager;
    std::array<EntityTracker, 2> entityTrackers;
    ServerWorld* worlds[2] = {nullptr, nullptr};

    [[nodiscard]] bool isOperator(const std::string& name) const
    {
        return playerManager.isOperator(name);
    }

    ServerWorld* getWorld(int dimensionId);
    EntityTracker& getEntityTracker(int dimensionId);
    void tick();
};

} // namespace server
} // namespace net::minecraft
