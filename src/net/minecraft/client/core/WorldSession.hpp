#pragma once
#include <memory>
#include <string>
#include <vector>

namespace net::minecraft {
class World;
class WorldStorage;
}  // namespace net::minecraft

namespace net::minecraft::entity::player {
class ClientPlayerEntity;
class PlayerEntity;
}  // namespace net::minecraft::entity::player

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::core {
/// Owns client-side world/player/storage lifetime and enforces renderer teardown ordering.
/// Anchor: Minecraft.cpp L1092–1185 (setWorld), L1173–1177 (clearWorld invariant).
class WorldSession {
   public:
    WorldSession() = default;
    ~WorldSession() = default;
    void setWorld(Minecraft& client,
                  World* worldIn,
                  const std::string& message,
                  entity::player::PlayerEntity* existingPlayer,
                  bool skipTerrainPrepare = false);
    /// worldRenderer->setWorld(nullptr) before ownedPlayer_/ownedWorld_ destruction.
    void clearWorld(Minecraft& client);
    void unloadWorld(Minecraft& client);

    [[nodiscard]] World* ownedWorld() const noexcept {
        return ownedWorld_.get();
    }

    /// Previous dimension world kept alive across portal travel (Java GC timing).
    [[nodiscard]] World* parkedDimensionWorld() const noexcept {
        return parkedDimensionWorld_.get();
    }

    [[nodiscard]] WorldStorage* ownedWorldStorage() const noexcept {
        return ownedWorldStorage_.get();
    }

    [[nodiscard]] entity::player::ClientPlayerEntity* ownedPlayer() const noexcept {
        return ownedPlayer_.get();
    }

    std::unique_ptr<WorldStorage>& ownedWorldStorageMut() noexcept {
        return ownedWorldStorage_;
    }

    std::unique_ptr<World>& ownedWorldMut() noexcept {
        return ownedWorld_;
    }

    std::unique_ptr<World>& parkedDimensionWorldMut() noexcept {
        return parkedDimensionWorld_;
    }

    std::unique_ptr<entity::player::ClientPlayerEntity>& ownedPlayerMut() noexcept {
        return ownedPlayer_;
    }

    void prepareWorld(Minecraft& client, const std::string& worldName);
    void convertAndSaveWorld(Minecraft& client, const std::string& worldName, const std::string& name);
    void tickJoinPlayerCounter(Minecraft& client);
    /// Save and detach the active local world so an integrated server can take session.lock.
    bool parkLocalWorldForRemoteHandoff(Minecraft& client);
    /// Restore a parked local world after a failed or canceled LAN handoff.
    bool restoreParkedLocalWorld(Minecraft& client);
    /// Drop the parked local copy once the client has joined the hosted server world.
    void commitRemoteHandoff();

    [[nodiscard]] bool hasParkedLocalHandoff() const noexcept {
        return parkedHandoffWorld_ != nullptr;
    }

    void suppressNextRemoteHandoffSave() noexcept {
        suppressNextRemoteHandoffSave_ = true;
    }

    void clearRemoteHandoffSaveSuppression() noexcept {
        suppressNextRemoteHandoffSave_ = false;
    }

   private:
    std::unique_ptr<WorldStorage> ownedWorldStorage_;
    std::unique_ptr<World> ownedWorld_;
    std::unique_ptr<World> parkedDimensionWorld_;
    std::unique_ptr<WorldStorage> parkedHandoffWorldStorage_;
    std::unique_ptr<World> parkedHandoffWorld_;
    std::unique_ptr<entity::player::ClientPlayerEntity> ownedPlayer_;
    bool suppressNextRemoteHandoffSave_ = false;
};
}  // namespace net::minecraft::client::core
