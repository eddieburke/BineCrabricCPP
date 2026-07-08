#include "net/minecraft/world/WorldProperties.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"

namespace net::minecraft {
NbtCompound WorldProperties::asNbt(const std::vector<entity::player::PlayerEntity*>& players) const {
    const entity::player::PlayerEntity* player = players.empty() ? nullptr : players.front();
    if (player == nullptr) {
        return asNbt(static_cast<const NbtCompound*>(nullptr));
    }
    NbtCompound playerNbt;
    player->writeNbt(playerNbt);
    return asNbt(&playerNbt);
}
}  // namespace net::minecraft
