#pragma once

#include <string>
#include "net/minecraft/entity/EntityTypes.hpp"

namespace net::minecraft::block::entity {
class BlockEntity;
}

namespace net::minecraft {


class GameEventListener {
public:
    virtual ~GameEventListener() = default;

    virtual void blockUpdate(int x, int y, int z) = 0;
    virtual void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) = 0;
    virtual void playSound(const std::string& name, double x, double y, double z, float volume, float pitch) = 0;
    virtual void addParticle(const std::string& name, double x, double y, double z, double vx, double vy, double vz) = 0;
    virtual void notifyEntityAdded(Entity* entity) = 0;
    virtual void notifyEntityRemoved(Entity* entity) = 0;
    virtual void notifyAmbientDarknessChanged() = 0;
    virtual void playStreaming(const std::string& name, int x, int y, int z) = 0;
    virtual void updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity) = 0;
    virtual void onEntityPickup(Entity* /*entity*/, PlayerEntity* /*collector*/) {}
    virtual void worldEvent(PlayerEntity* player, int type, int x, int y, int z, int data) = 0;
};

} // namespace net::minecraft
