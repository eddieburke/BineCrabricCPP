#pragma once

#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtList.hpp"

#include <cstddef>
#include <vector>

namespace net::minecraft::block::entity::inventory_util {

inline void readInventoryItems(const NbtCompound& nbt, std::vector<ItemStack>& inventory, bool unsignedSlot = true)
{
    inventory.assign(inventory.size(), {});
    if (!nbt.contains("Items")) {
        return;
    }
    const NbtList items = nbt.getList("Items");
    for (const Nbt& entryTag : items.entries()) {
        if (!entryTag.isCompound()) {
            continue;
        }
        const NbtCompound entry(entryTag);
        int slot = 0;
        if (unsignedSlot) {
            slot = entry.getByte("Slot") & 0xFF;
        } else {
            slot = entry.getByte("Slot");
        }
        if (slot >= 0 && slot < static_cast<int>(inventory.size())) {
            inventory[static_cast<std::size_t>(slot)] = ItemStack::fromNbt(entry);
        }
    }
}

inline void writeInventoryItems(NbtCompound& nbt, const std::vector<ItemStack>& inventory)
{
    NbtList items;
    auto& list = items.storage().asList();
    for (std::size_t i = 0; i < inventory.size(); ++i) {
        if (inventory[i].empty()) {
            continue;
        }
        NbtCompound entry = inventory[i].toNbt();
        entry.putByte("Slot", static_cast<std::int8_t>(i));
        list.push_back(entry.storage());
    }
    nbt.put("Items", items);
}

[[nodiscard]] bool canPlayerUseBlockEntity(const BlockEntity& self, PlayerEntity* player);

} // namespace net::minecraft::block::entity::inventory_util
