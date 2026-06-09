#pragma once

namespace net::minecraft::entity {
class Entity;
}

namespace net::minecraft::entity::player {
class ServerPlayerEntity;
}

namespace net::minecraft::server {

class MinecraftServer;

// Entity visibility tracking (packet sync stubbed until network layer).
class EntityTracker {
public:
    EntityTracker(MinecraftServer* server, int dimensionId);

    void onEntityAdded(entity::Entity* entity);
    void onEntityRemoved(entity::Entity* entity);
    void tick();
    void removeListener(entity::player::ServerPlayerEntity* player);
    void sendToListeners(entity::Entity* entity);
    void sendToAround(entity::Entity* entity);

private:
    MinecraftServer* server_ = nullptr;
    int dimensionId_ = 0;
};

} // namespace server
