#pragma once
#include <memory>
#include <unordered_set>
#include <vector>

#include "net/minecraft/server/entity/EntityTrackerEntry.hpp"
#include "net/minecraft/util/IntHashMap.hpp"

namespace net::minecraft::entity {
class Entity;
}

namespace net::minecraft::entity::player {
class ServerPlayerEntity;
}

namespace net::minecraft::server {
class MinecraftServer;

// Port of net.minecraft.server.entity.EntityTracker (beta 1.7.3): per-dimension
// entity replication. Decides tracking range/frequency per entity type and drives
// EntityTrackerEntry updates each tick.
class EntityTracker {
   public:
    EntityTracker(MinecraftServer* server, int dimensionId);
    void onEntityAdded(::net::minecraft::entity::Entity* entity);
    void startTracking(::net::minecraft::entity::Entity* entity, int trackedDistance, int trackingFrequency);
    void startTracking(::net::minecraft::entity::Entity* entity,
                       int trackedDistance,
                       int trackingFrequency,
                       bool alwaysUpdateVelocity);
    void onEntityRemoved(::net::minecraft::entity::Entity* entity);
    void tick();
    void removeListener(::net::minecraft::entity::player::ServerPlayerEntity* player);

    template <typename PacketT>
    void sendToListeners(::net::minecraft::entity::Entity* entity, const PacketT& packet) {
        if (entity == nullptr) {
            return;
        }
        if (entity::EntityTrackerEntry* entry = entriesById_.get(entity->id)) {
            entry->sendToListeners(packet);
        }
    }

    template <typename PacketT>
    void sendToAround(::net::minecraft::entity::Entity* entity, const PacketT& packet) {
        if (entity == nullptr) {
            return;
        }
        if (entity::EntityTrackerEntry* entry = entriesById_.get(entity->id)) {
            entry->sendToAround(packet);
        }
    }

   private:
    [[nodiscard]] std::vector<::net::minecraft::entity::player::ServerPlayerEntity*> dimensionPlayers() const;
    std::vector<std::unique_ptr<entity::EntityTrackerEntry>> entries_;
    util::IntHashMap<entity::EntityTrackerEntry*> entriesById_;
    MinecraftServer* server_ = nullptr;
    int viewDistance_ = 0;
    int dimensionId_ = 0;
};
}  // namespace net::minecraft::server
