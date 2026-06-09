#include "net/minecraft/entity/player/PlayerInventory.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::entity::player {

void PlayerInventory::damageArmor(int amount)
{
    for (ItemStack& piece : armor) {
        if (piece.empty()) {
            continue;
        }
        if (dynamic_cast<const item::ArmorItem*>(piece.getItem()) == nullptr) {
            continue;
        }
        piece.damageStack(amount, player_);
        if (piece.count == 0) {
            piece.onRemoved(player_);
            piece = {};
        }
    }
}

bool PlayerInventory::canPlayerUse(PlayerEntity* player) const
{
    if (player_ != nullptr && player_->dead) {
        return false;
    }
    if (player == nullptr || player_ == nullptr) {
        return true;
    }
    return player->getSquaredDistance(*player_) <= 64.0;
}

void PlayerInventory::inventoryTick()
{
    if (player_ == nullptr) {
        return;
    }
    for (std::size_t slot = 0; slot < main.size(); ++slot) {
        if (main[slot].empty()) {
            continue;
        }
        main[slot].inventoryTick(
            player_->world,
            player_,
            static_cast<int>(slot),
            selectedSlot == static_cast<int>(slot));
    }
}

void PlayerInventory::dropInventory()
{
    if (player_ == nullptr) {
        main.fill({});
        armor.fill({});
        return;
    }
    for (ItemStack& stack : main) {
        if (stack.empty()) {
            continue;
        }
        player_->dropItem(std::move(stack), true);
    }
    for (ItemStack& stack : armor) {
        if (stack.empty()) {
            continue;
        }
        player_->dropItem(std::move(stack), true);
    }
    main.fill({});
    armor.fill({});
}

void PlayerInventory::setCursorStack(ItemStack stack)
{
    cursorStack_ = stack;
    if (player_ != nullptr) {
        player_->onCursorStackChanged(cursorStack_);
    }
}

} // namespace net::minecraft::entity::player
