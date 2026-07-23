#pragma once
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft::inventory {
// Insert `stack` into the player's main inventory, merging into existing stacks
// first and then filling empty slots. Returns whatever did not fit (empty stack
// if everything was consumed). Use this when the caller wants to handle overflow.
ItemStack offerToPlayer(entity::player::PlayerEntity& player, ItemStack stack);
// Like offerToPlayer, but drops any leftover at the player's feet. This is the
// "give it to the player no matter what" path used by creative item panels (TMI)
// and by containers that must not destroy their contents on close (repair table).
void giveToPlayer(entity::player::PlayerEntity& player, ItemStack stack);
} // namespace net::minecraft::inventory
