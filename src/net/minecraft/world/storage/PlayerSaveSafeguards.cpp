#include "net/minecraft/world/storage/PlayerSaveSafeguards.hpp"

#include <algorithm>
#include <cmath>

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/nbt/NbtList.hpp"

namespace net::minecraft::world::storage {
namespace {
[[nodiscard]] bool isDefaultSpawnPosition(const NbtCompound& playerNbt) {
    if (!playerNbt.contains("Pos")) {
        return true;
    }
    const NbtList pos = playerNbt.getList("Pos");
    if (pos.size() < 3) {
        return true;
    }
    const auto& entries = pos.entries();
    const double x = entries[0].asDouble();
    const double y = entries[1].asDouble();
    const double z = entries[2].asDouble();
    return std::abs(x) < 0.5 && std::abs(y) < 0.5 && std::abs(z) < 0.5;
}
}  // namespace

int countInventoryStacks(const NbtCompound& playerNbt) {
    if (!playerNbt.contains("Inventory")) {
        return 0;
    }
    int stacks = 0;
    const NbtList inventory = playerNbt.getList("Inventory");
    for (const Nbt& entryTag : inventory.entries()) {
        if (!entryTag.isCompound()) {
            continue;
        }
        const NbtCompound entry(entryTag);
        if (entry.getShort("id") <= 0 || entry.getByte("Count") <= 0) {
            continue;
        }
        ++stacks;
    }
    return stacks;
}

bool hasSavedPosition(const NbtCompound& playerNbt) {
    return !isDefaultSpawnPosition(playerNbt);
}

NbtCompound applyPlayerSaveSafeguards(NbtCompound proposed, const NbtCompound& previous) {
    if (countInventoryStacks(proposed) == 0 && countInventoryStacks(previous) > 0) {
        proposed.put("Inventory", previous.getList("Inventory"));
        if (previous.contains("SelectedItemSlot")) {
            proposed.putInt("SelectedItemSlot", previous.getInt("SelectedItemSlot"));
        }
    }
    if (isDefaultSpawnPosition(proposed) && hasSavedPosition(previous)) {
        proposed.put("Pos", previous.getList("Pos"));
        if (previous.contains("Rotation")) {
            proposed.put("Rotation", previous.getList("Rotation"));
        }
        if (previous.contains("Motion")) {
            proposed.put("Motion", previous.getList("Motion"));
        }
        if (previous.contains("Dimension")) {
            proposed.putInt("Dimension", previous.getInt("Dimension"));
        }
    }
    if (proposed.contains("SelectedItemSlot")) {
        const int slot = proposed.getInt("SelectedItemSlot");
        proposed.putInt("SelectedItemSlot", std::clamp(slot, 0, 8));
    }
    return proposed;
}

NbtCompound buildSafeguardedPlayerNbt(const entity::player::PlayerEntity& player, const NbtCompound* previous) {
    NbtCompound nbt;
    player.writeNbt(nbt);
    if (nbt.contains("SelectedItemSlot")) {
        nbt.putInt("SelectedItemSlot", std::clamp(player.inventory.selectedSlot, 0, 8));
    }
    if (previous != nullptr) {
        nbt = applyPlayerSaveSafeguards(std::move(nbt), *previous);
    }
    return nbt;
}
}  // namespace net::minecraft::world::storage
