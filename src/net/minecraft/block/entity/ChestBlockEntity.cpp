#include "net/minecraft/block/entity/ChestBlockEntity.hpp"
namespace net::minecraft::block::entity {
ItemStack ChestBlockEntity::getStack(std::size_t slot) const {
 if(slot >= inventory_.size()) {
  return {};
 }
 return inventory_[slot];
}
ItemStack ChestBlockEntity::removeStack(std::size_t slot, int amount) {
 if(slot >= inventory_.size()) {
  return {};
 }
 ItemStack& stack = inventory_[slot];
 if(stack.empty()) {
  return {};
 }
 if(stack.count <= amount) {
  ItemStack removed = stack;
  stack = {};
  markDirty();
  return removed;
 }
 ItemStack removed = stack.split(amount);
 if(stack.count == 0) {
  stack = {};
 }
 markDirty();
 return removed;
}
void ChestBlockEntity::setStack(std::size_t slot, ItemStack stack) {
 if(slot >= inventory_.size()) {
  return;
 }
 if(!stack.empty() && stack.count > getMaxCountPerStack()) {
  stack.count = getMaxCountPerStack();
 }
 inventory_[slot] = std::move(stack);
 markDirty();
}
std::string ChestBlockEntity::getName() const {
 return "Chest";
}
void ChestBlockEntity::readNbt(const NbtCompound& nbt) {
 BlockEntity::readNbt(nbt);
 inventory_.assign(size(), {});
 inventory_util::readInventoryItems(nbt, inventory_);
}
void ChestBlockEntity::writeNbt(NbtCompound& nbt) const {
 BlockEntity::writeNbt(nbt);
 inventory_util::writeInventoryItems(nbt, inventory_);
}
int ChestBlockEntity::getMaxCountPerStack() const {
 return 64;
}
void ChestBlockEntity::markDirty() {
 BlockEntity::markDirty();
}
bool ChestBlockEntity::canPlayerUse(PlayerEntity* player) const {
 return inventory_util::canPlayerUseBlockEntity(*this, player);
}
} // namespace net::minecraft::block::entity
