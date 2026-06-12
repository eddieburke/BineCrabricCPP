#include "net/minecraft/server/MinecraftServer.hpp"

#include "net/minecraft/world/ServerWorld.hpp"

namespace net::minecraft::server {

MinecraftServer::MinecraftServer()
    : playerManager(this),
      entityTrackers{EntityTracker(this, 0), EntityTracker(this, -1)}
{
}

ServerWorld* MinecraftServer::getWorld(int dimensionId)
{
    if (dimensionId == -1) {
        return worlds[1];
    }
    return worlds[0];
}

EntityTracker& MinecraftServer::getEntityTracker(int dimensionId)
{
    if (dimensionId == -1) {
        return entityTrackers[1];
    }
    return entityTrackers[0];
}

void MinecraftServer::tick()
{
    ++ticks;
    for (int i = 0; i < 2; ++i) {
        if (i != 0 && !allowNether) {
            continue;
        }
        ServerWorld* world = worlds[i];
        if (world == nullptr) {
            continue;
        }
        world->tick();
        world->doLightingUpdates();
        world->tickEntities();
    }
    playerManager.updateAllChunks();
    entityTrackers[0].tick();
    entityTrackers[1].tick();
}

} // namespace net::minecraft::server
