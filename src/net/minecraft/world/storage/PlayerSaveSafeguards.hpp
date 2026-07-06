#pragma once
#include "net/minecraft/nbt/NbtCompound.hpp"
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft::world::storage {
[[nodiscard]] int countInventoryStacks(const NbtCompound& playerNbt);
[[nodiscard]] bool hasSavedPosition(const NbtCompound& playerNbt);
[[nodiscard]] NbtCompound applyPlayerSaveSafeguards(NbtCompound proposed, const NbtCompound& previous);
[[nodiscard]] NbtCompound buildSafeguardedPlayerNbt(const entity::player::PlayerEntity& player,
                                                    const NbtCompound* previous);
} // namespace net::minecraft::world::storage
