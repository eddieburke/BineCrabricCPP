#pragma once
#include "net/minecraft/inventory/Inventory.hpp"
namespace net::minecraft::screen::slot {
class Slot {
 public:
 Slot(Inventory* inventory, int index, int x, int y) : inventory_(inventory), index_(index), x(x), y(y) {
 }
 virtual ~Slot() = default;
 virtual void onTakeItem(const ItemStack& stack) {
  (void)stack;
  markDirty();
 }
 [[nodiscard]] virtual bool canInsert(const ItemStack& stack) const {
  (void)stack;
  return true;
 }
 [[nodiscard]] ItemStack getStack() const {
  if(inventory_ == nullptr || index_ < 0) {
   return {};
  }
  const std::size_t slot = static_cast<std::size_t>(index_);
  if(slot >= inventory_->size()) {
   return {};
  }
  return inventory_->getStack(slot);
 }
 [[nodiscard]] bool hasStack() const {
  return !getStack().empty();
 }
 virtual void setStack(ItemStack stack) {
  if(inventory_ == nullptr || index_ < 0) {
   return;
  }
  const std::size_t slot = static_cast<std::size_t>(index_);
  if(slot >= inventory_->size()) {
   return;
  }
  inventory_->setStack(slot, std::move(stack));
  markDirty();
 }
 virtual void markDirty() {
  if(inventory_ != nullptr) {
   inventory_->markDirty();
  }
 }
 [[nodiscard]] virtual int getMaxItemCount() const {
  return inventory_ == nullptr ? 64 : inventory_->getMaxCountPerStack();
 }
 [[nodiscard]] virtual int getBackgroundTextureId() const {
  return -1;
 }
 [[nodiscard]] ItemStack takeStack(int amount) {
  if(inventory_ == nullptr || index_ < 0) {
   return {};
  }
  const std::size_t slot = static_cast<std::size_t>(index_);
  if(slot >= inventory_->size()) {
   return {};
  }
  return inventory_->removeStack(slot, amount);
 }
 [[nodiscard]] bool equals(const Inventory* inventory, int index) const {
  return inventory == inventory_ && index == index_;
 }
 Inventory* inventory_ = nullptr;
 int index_ = 0;
 int id = 0;
 int x = 0;
 int y = 0;
};
} // namespace net::minecraft::screen::slot
