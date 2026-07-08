#pragma once
#include <memory>
#include <unordered_set>
#include <vector>

#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"

namespace net::minecraft::server::entity {
// Port of net.minecraft.server.entity.EntityTrackerEntry (beta 1.7.3): per-entity
// replication state. Owns the listener set and decides, each tick, between relative
// move, rotate, teleport and velocity packets, plus DataTracker dirty flushes.
class EntityTrackerEntry {
   public:
    EntityTrackerEntry(::net::minecraft::entity::Entity* entity,
                       int trackedDistance,
                       int trackingFrequency,
                       bool alwaysUpdateVelocity);
    ::net::minecraft::entity::Entity* currentTrackedEntity = nullptr;
    int trackedDistance = 0;
    int trackingFrequency = 0;
    int lastX = 0;
    int lastY = 0;
    int lastZ = 0;
    int lastYaw = 0;
    int lastPitch = 0;
    double velocityX = 0.0;
    double velocityY = 0.0;
    double velocityZ = 0.0;
    int ticks = 0;
    bool newPlayerDataUpdated = false;
    std::unordered_set<::net::minecraft::entity::player::ServerPlayerEntity*> listeners;
    void notifyNewLocation(const std::vector<::net::minecraft::entity::player::ServerPlayerEntity*>& players);
    void updateListener(::net::minecraft::entity::player::ServerPlayerEntity* player);
    void updateListeners(const std::vector<::net::minecraft::entity::player::ServerPlayerEntity*>& players);
    void notifyEntityRemoved();
    void notifyEntityRemoved(::net::minecraft::entity::player::ServerPlayerEntity* player);
    void removeListener(::net::minecraft::entity::player::ServerPlayerEntity* player);

    template <typename PacketT>
    void sendToListeners(const PacketT& packet) {
        for (auto* listener : listeners) {
            if (listener != nullptr && listener->networkHandler != nullptr) {
                listener->networkHandler->sendPacket(packet);
            }
        }
    }

    template <typename PacketT>
    void sendToAround(const PacketT& packet) {
        sendToListeners(packet);
        if (auto* trackedPlayer =
                dynamic_cast<::net::minecraft::entity::player::ServerPlayerEntity*>(currentTrackedEntity)) {
            if (trackedPlayer->networkHandler != nullptr) {
                trackedPlayer->networkHandler->sendPacket(packet);
            }
        }
    }

   private:
    [[nodiscard]] std::unique_ptr<Packet> createAddEntityPacket() const;
    double x_ = 0.0;
    double y_ = 0.0;
    double z_ = 0.0;
    bool isInitialized_ = false;
    bool alwaysUpdateVelocity_ = false;
    int ticksSinceLastDismount_ = 0;
};
}  // namespace net::minecraft::server::entity
