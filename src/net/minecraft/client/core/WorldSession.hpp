#pragma once

#include <memory>
#include <string>

namespace net::minecraft {
class World;
class WorldStorage;
}

namespace net::minecraft::entity::player {
class ClientPlayerEntity;
class PlayerEntity;
}

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::core {

/// Owns client-side world/player/storage lifetime and enforces renderer teardown ordering.
/// Anchor: Minecraft.cpp L1092–1185 (setWorld), L1173–1177 (clearWorld invariant).
class WorldSession {
public:
    WorldSession() = default;

    void setWorld(Minecraft& client, World* worldIn);
    void setWorld(Minecraft& client, World* worldIn, const std::string& message);
    void setWorld(Minecraft& client, World* worldIn, const std::string& message, entity::player::PlayerEntity* existingPlayer);

    /// worldRenderer->setWorld(nullptr) before ownedPlayer_/ownedWorld_ destruction.
    void clearWorld(Minecraft& client);

    [[nodiscard]] World* ownedWorld() const noexcept { return ownedWorld_.get(); }
    [[nodiscard]] WorldStorage* ownedWorldStorage() const noexcept { return ownedWorldStorage_.get(); }
    [[nodiscard]] entity::player::ClientPlayerEntity* ownedPlayer() const noexcept { return ownedPlayer_.get(); }

    std::unique_ptr<WorldStorage>& ownedWorldStorageMut() noexcept { return ownedWorldStorage_; }
    std::unique_ptr<World>& ownedWorldMut() noexcept { return ownedWorld_; }
    std::unique_ptr<entity::player::ClientPlayerEntity>& ownedPlayerMut() noexcept { return ownedPlayer_; }

    void prepareWorld(Minecraft& client, const std::string& worldName);
    void convertAndSaveWorld(Minecraft& client, const std::string& worldName, const std::string& name);
    void tickJoinPlayerCounter(Minecraft& client);

private:
    int joinPlayerCounter_ = 0;
    std::unique_ptr<WorldStorage> ownedWorldStorage_;
    std::unique_ptr<World> ownedWorld_;
    std::unique_ptr<entity::player::ClientPlayerEntity> ownedPlayer_;
};

} // namespace net::minecraft::client::core
