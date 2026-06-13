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

    virtual void blockUpdate(int x, int y, int z)
    {
        (void)x;
        (void)y;
        (void)z;
    }

    virtual void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ)
    {
        (void)minX;
        (void)minY;
        (void)minZ;
        (void)maxX;
        (void)maxY;
        (void)maxZ;
    }

    virtual void playSound(const std::string& name, double x, double y, double z, float volume, float pitch)
    {
        (void)name;
        (void)x;
        (void)y;
        (void)z;
        (void)volume;
        (void)pitch;
    }

    virtual void addParticle(
        const std::string& name, double x, double y, double z, double vx, double vy, double vz)
    {
        (void)name;
        (void)x;
        (void)y;
        (void)z;
        (void)vx;
        (void)vy;
        (void)vz;
    }

    virtual void notifyEntityAdded(Entity* entity) { (void)entity; }
    virtual void notifyEntityRemoved(Entity* entity) { (void)entity; }
    virtual void notifyAmbientDarknessChanged() {}
    virtual void playStreaming(const std::string& name, int x, int y, int z)
    {
        (void)name;
        (void)x;
        (void)y;
        (void)z;
    }

    virtual void updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity)
    {
        (void)x;
        (void)y;
        (void)z;
        (void)blockEntity;
    }

    virtual void onEntityPickup(Entity* entity, PlayerEntity* collector)
    {
        (void)entity;
        (void)collector;
    }

    virtual void blockBreakParticles(int x, int y, int z, int blockId, int blockMeta)
    {
        (void)x;
        (void)y;
        (void)z;
        (void)blockId;
        (void)blockMeta;
    }
};

} // namespace net::minecraft
