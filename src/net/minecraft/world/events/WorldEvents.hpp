#pragma once

#include <string>
#include <vector>

#include "net/minecraft/world/event/listener/GameEventListener.hpp"

namespace net::minecraft {

class World;

namespace block::entity {
class BlockEntity;
}

/**
 * Owns GameEventListener registration and fan-out for a World instance.
 * Standalone module (M3); World will delegate here once wired.
 */
class WorldEvents {
public:
    explicit WorldEvents(World& world);

    WorldEvents(const WorldEvents&) = delete;
    WorldEvents& operator=(const WorldEvents&) = delete;

    void addEventListener(GameEventListener* listener);
    void removeEventListener(GameEventListener* listener);

    void blockUpdateEvent(int x, int y, int z);
    void blockUpdate(int x, int y, int z, int blockId);
    void setBlockDirty(int x, int y, int z);

    void notifyEntityAdded(Entity* entity);
    void notifyEntityRemoved(Entity* entity);
    void notifyEntityPickup(Entity* entity, PlayerEntity* collector);
    void notifyAmbientDarknessChanged();

    void playSound(double x, double y, double z, const std::string& name, float volume, float pitch);
    void playSound(Entity* source, const std::string& name, float volume, float pitch);
    void playSound(PlayerEntity* player, const std::string& name, float volume, float pitch);
    void playStreaming(const std::string& name, int x, int y, int z);

    void worldEvent(PlayerEntity* player, int type, int x, int y, int z, int data);
    void worldEvent(int type, int x, int y, int z, int data) { worldEvent(nullptr, type, x, y, z, data); }

    void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
    void setBlocksDirtyColumn(int x, int z, int minY, int maxY);

    void updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity);
    void addParticle(
        const std::string& name,
        double px,
        double py,
        double pz,
        double vx,
        double vy,
        double vz);

private:
    World& world_;
    std::vector<GameEventListener*> eventListeners_;
};

} // namespace net::minecraft
