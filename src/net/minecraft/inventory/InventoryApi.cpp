#include "net/minecraft/inventory/InventoryApi.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"

namespace net::minecraft::inventory {
ItemStack offerToPlayer(entity::player::PlayerEntity& player, ItemStack stack) {
    if (stack.empty()) {
        return {};
    }
    player.inventory.addStack(stack);  // mutates stack.count down to whatever didn't fit
    if (stack.count <= 0) {
        return {};
    }
    return stack;
}

void giveToPlayer(entity::player::PlayerEntity& player, ItemStack stack) {
    ItemStack remainder = offerToPlayer(player, std::move(stack));
    if (!remainder.empty()) {
        player.dropItem(remainder);
    }
}
}  // namespace net::minecraft::inventory
