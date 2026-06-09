#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"

namespace net::minecraft::block::entity {

ItemStack DispenserBlockEntity::getStack(std::size_t slot) const
{
    if (slot >= inventory_.size()) {
        return {};
    }
    return inventory_[slot];
}

ItemStack DispenserBlockEntity::removeStack(std::size_t slot, int amount)
{
    if (slot >= inventory_.size()) {
        return {};
    }
    ItemStack& stack = inventory_[slot];
    if (stack.empty()) {
        return {};
    }
    if (stack.count <= amount) {
        ItemStack removed = stack;
        stack = {};
        markDirty();
        return removed;
    }
    ItemStack removed = stack.split(amount);
    if (stack.count == 0) {
        stack = {};
    }
    markDirty();
    return removed;
}

ItemStack DispenserBlockEntity::getItemToDispense()
{
    int chosen = -1;
    int seen = 1;
    for (std::size_t i = 0; i < inventory_.size(); ++i) {
        if (inventory_[i].empty()) {
            continue;
        }
        std::uniform_int_distribution<int> distribution(0, seen++ - 1);
        if (distribution(random_) == 0) {
            chosen = static_cast<int>(i);
        }
    }
    if (chosen >= 0) {
        return removeStack(static_cast<std::size_t>(chosen), 1);
    }
    return {};
}

void DispenserBlockEntity::setStack(std::size_t slot, ItemStack stack)
{
    if (slot >= inventory_.size()) {
        return;
    }
    if (!stack.empty() && stack.count > getMaxCountPerStack()) {
        stack.count = getMaxCountPerStack();
    }
    inventory_[slot] = std::move(stack);
    markDirty();
}

std::string DispenserBlockEntity::getName() const
{
    return "Trap";
}

void DispenserBlockEntity::readNbt(const NbtCompound& nbt)
{
    BlockEntity::readNbt(nbt);
    inventory_.assign(inventory_.size(), {});
    inventory_util::readInventoryItems(nbt, inventory_);
}

void DispenserBlockEntity::writeNbt(NbtCompound& nbt) const
{
    BlockEntity::writeNbt(nbt);
    inventory_util::writeInventoryItems(nbt, inventory_);
}

int DispenserBlockEntity::getMaxCountPerStack() const
{
    return 64;
}

void DispenserBlockEntity::markDirty()
{
    BlockEntity::markDirty();
}

bool DispenserBlockEntity::canPlayerUse(PlayerEntity* player) const
{
    return inventory_util::canPlayerUseBlockEntity(*this, player);
}

} // namespace net::minecraft::block::entity
