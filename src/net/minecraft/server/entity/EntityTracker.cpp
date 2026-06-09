#include "net/minecraft/server/entity/EntityTracker.hpp"

#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"

namespace net::minecraft::server {

EntityTracker::EntityTracker(MinecraftServer* server, int dimensionId)
    : server_(server),
      dimensionId_(dimensionId)
{
    (void)server_;
    (void)dimensionId_;
}

void EntityTracker::onEntityAdded(Entity* entity)
{
    (void)entity;
}

void EntityTracker::onEntityRemoved(Entity* entity)
{
    (void)entity;
}

void EntityTracker::tick()
{
}

void EntityTracker::removeListener(entity::player::ServerPlayerEntity* player)
{
    (void)player;
}

void EntityTracker::sendToListeners(Entity* entity)
{
    (void)entity;
}

void EntityTracker::sendToAround(Entity* entity)
{
    (void)entity;
}

} // namespace net::minecraft::server
