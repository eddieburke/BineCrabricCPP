#pragma once

#include "net/minecraft/client/InteractionManager.hpp"

namespace net::minecraft::client {

class TestInteractionManager : public InteractionManager {
public:
    explicit TestInteractionManager(Minecraft* minecraft);

    void preparePlayerRespawn(PlayerEntity* player);
    bool canBeRendered();
    void setWorld(World* world);
    void tick();
};

} // namespace net::minecraft::client
