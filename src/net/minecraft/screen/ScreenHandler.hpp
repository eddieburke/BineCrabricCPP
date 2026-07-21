#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <unordered_set>
#include <vector>
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"
namespace net::minecraft::screen {
class ScreenHandler;
class ScreenHandlerListener {
 public:
 virtual ~ScreenHandlerListener() = default;
 virtual void onSlotUpdate(ScreenHandler& handler, int slot, const ItemStack& stack) = 0;
 virtual void onContentsUpdate(ScreenHandler& handler, const std::vector<ItemStack>& stacks) = 0;
 virtual void onPropertyUpdate(ScreenHandler& handler, int property, int value) = 0;
};
class ScreenHandler {
 public:
 virtual ~ScreenHandler() = default;
 std::vector<ItemStack> trackedStacks;
 std::vector<slot::Slot*> slots;
 int syncId = 0;

 protected:
 std::vector<ScreenHandlerListener*> listeners_;
 void addSlot(slot::Slot* slot) {
  slot->id = static_cast<int>(slots.size());
  slots.push_back(slot);
  trackedStacks.emplace_back();
 }
 [[nodiscard]] bool hasSlot(int index) const {
  return index >= 0 && index < static_cast<int>(slots.size());
 }
 void insertItem(ItemStack& stack, int startIndex, int endIndex, bool fromLast) {
  if(stack.empty() || startIndex < 0 || endIndex < 0 || startIndex > endIndex ||
     endIndex > static_cast<int>(slots.size())) {
   return;
  }
  int slotIndex = startIndex;
  if(fromLast) {
   slotIndex = endIndex - 1;
  }
  if(stack.isStackable()) {
   while(stack.count > 0 && ((!fromLast && slotIndex < endIndex) || (fromLast && slotIndex >= startIndex))) {
    slot::Slot* currentSlot = getSlot(slotIndex);
    if(currentSlot != nullptr) {
     ItemStack slotStack = currentSlot->getStack();
     if(!slotStack.empty() && slotStack.itemId == stack.itemId &&
        (!stack.hasSubtypes() || slotStack.getDamage() == stack.getDamage())) {
      int combined = slotStack.count + stack.count;
      if(combined <= stack.getMaxCount()) {
       stack.count = 0;
       slotStack.count = combined;
       currentSlot->setStack(slotStack);
      } else if(slotStack.count < stack.getMaxCount()) {
       stack.count -= stack.getMaxCount() - slotStack.count;
       slotStack.count = stack.getMaxCount();
       currentSlot->setStack(slotStack);
      }
     }
    }
    if(fromLast) {
     --slotIndex;
    } else {
     ++slotIndex;
    }
   }
  }
  if(stack.count > 0) {
   slotIndex = fromLast ? endIndex - 1 : startIndex;
   while((!fromLast && slotIndex < endIndex) || (fromLast && slotIndex >= startIndex)) {
    slot::Slot* currentSlot = getSlot(slotIndex);
    if(currentSlot != nullptr) {
     ItemStack slotStack = currentSlot->getStack();
     if(slotStack.empty()) {
      currentSlot->setStack(stack.copy());
      stack.count = 0;
      break;
     }
    }
    if(fromLast) {
     --slotIndex;
    } else {
     ++slotIndex;
    }
   }
  }
 }
 // ---- Reusable screen-handler building blocks --------------------------------
 // These remove the layout/shift-click boilerplate that every container (vanilla
 // and modded) used to copy verbatim. A handler now just declares its slots and
 // describes where a shift-clicked stack should go.
 // Adds the standard 36 player-inventory slots (3x9 main grid above a hotbar row)
 // and returns the slot index at which the player inventory begins.
 int addPlayerInventorySlots(Inventory* playerInventory, int originX = 8, int mainY = 84, int hotbarY = 142) {
  const int start = static_cast<int>(slots.size());
  if(playerInventory == nullptr) {
   return start;
  }
  for(int row = 0; row < 3; ++row) {
   for(int column = 0; column < 9; ++column) {
    addSlot(new slot::Slot(playerInventory, column + row * 9 + 9, originX + column * 18, mainY + row * 18));
   }
  }
  for(int column = 0; column < 9; ++column) {
   addSlot(new slot::Slot(playerInventory, column, originX + column * 18, hotbarY));
  }
  return start;
 }
 // Shared shift-click plumbing: pulls the stack from `slotIndex`, lets `route`
 // move it (via insertItem) into target regions, then writes back the remainder
 // and fires onTakeItem. Returns the original stack if anything moved, else {}.
 ItemStack performQuickMove(int slotIndex, const std::function<void(ItemStack&)>& route) {
  slot::Slot* currentSlot = getSlot(slotIndex);
  if(currentSlot == nullptr || !currentSlot->hasStack()) {
   return {};
  }
  ItemStack stack = currentSlot->getStack();
  ItemStack original = stack.copy();
  route(stack);
  if(stack.count == 0) {
   currentSlot->setStack({});
  } else {
   currentSlot->setStack(stack);
  }
  if(stack.count == original.count) {
   return {};
  }
  currentSlot->onTakeItem(stack);
  return original;
 }
 // Routing for workbench-style layouts (result/inputs above player main + hotbar):
 // a container stack spills into the whole player inventory, while main and hotbar
 // swap into each other. `playerStart` is the first player slot index (i.e. the
 // number of container slots). Result/output slots are handled by the caller.
 void routeToPlayerInventory(int slotIndex, ItemStack& stack, int playerStart) {
  const int end = static_cast<int>(slots.size());
  const int mainEnd = playerStart + 27;
  if(slotIndex < playerStart) {
   insertItem(stack, playerStart, end, false);
  } else if(slotIndex < mainEnd) {
   insertItem(stack, mainEnd, end, false);
  } else {
   insertItem(stack, playerStart, mainEnd, false);
  }
 }
 // Routing for storage-style layouts (chest/dispenser): container <-> player,
 // with container->player filling from the end like vanilla.
 void routeStorage(int slotIndex, ItemStack& stack, int containerEnd) {
  const int end = static_cast<int>(slots.size());
  if(slotIndex < containerEnd) {
   insertItem(stack, containerEnd, end, true);
  } else {
   insertItem(stack, 0, containerEnd, false);
  }
 }
 // Declare a workbench-style layout so the base quickMove() handles shift-clicks
 // with no per-handler override. `playerStart` is the first player slot index;
 // `outputSlots` lists result/output slot indices that spill into the player
 // inventory filling the hotbar last. Any per-slot side effects (e.g. consuming a
 // crafting grid) belong in the slot's onTakeItem, not here.
 void setupQuickMove(int playerStart, std::initializer_list<int> outputSlots = {}) {
  playerInventoryStart_ = playerStart;
  storageLayout_ = false;
  outputSlots_.assign(outputSlots);
 }
 // Declare a storage-style layout (chest/dispenser): container <-> player.
 void setupStorageQuickMove(int containerEnd) {
  playerInventoryStart_ = containerEnd;
  storageLayout_ = true;
  outputSlots_.clear();
 }

 public:
 virtual void addListener(ScreenHandlerListener* listener) {
  if(listener == nullptr || std::find(listeners_.begin(), listeners_.end(), listener) != listeners_.end()) {
   return;
  }
  listeners_.push_back(listener);
  listener->onContentsUpdate(*this, getStacks());
  sendContentUpdates();
 }
 // Single shift-click implementation for every container. Handlers describe their
 // layout once via setupQuickMove()/setupStorageQuickMove() instead of overriding
 // this. (Still virtual so an exotic container can opt out if it ever needs to.)
 [[nodiscard]] virtual ItemStack quickMove(int slotIndex) {
  if(playerInventoryStart_ < 0) {
   return {}; // unconfigured: no shift-transfer behavior
  }
  return performQuickMove(slotIndex, [&](ItemStack& stack) {
   if(storageLayout_) {
    routeStorage(slotIndex, stack, playerInventoryStart_);
   } else if(isOutputSlot(slotIndex)) {
    insertItem(stack, playerInventoryStart_, static_cast<int>(slots.size()), true);
   } else {
    routeToPlayerInventory(slotIndex, stack, playerInventoryStart_);
   }
  });
 }
 [[nodiscard]] virtual bool canUse(PlayerEntity* /*player*/) {
  return true;
 }
 virtual void sendContentUpdates() {
  for(std::size_t i = 0; i < slots.size(); ++i) {
   slot::Slot* currentSlot = slots[i];
   if(currentSlot == nullptr) {
    continue;
   }
   ItemStack stack = currentSlot->getStack();
   if(ItemStack::areEqual(trackedStacks[i], stack)) {
    continue;
   }
   trackedStacks[i] = stack.empty() ? ItemStack{} : stack.copy();
   for(ScreenHandlerListener* listener : listeners_) {
    if(listener != nullptr) {
     listener->onSlotUpdate(*this, static_cast<int>(i), trackedStacks[i]);
    }
   }
  }
 }
 virtual void onClosed(PlayerEntity* player);
 virtual void onSlotUpdate(Inventory* /*inventory*/) {
  sendContentUpdates();
 }
 [[nodiscard]] std::vector<ItemStack> getStacks() const {
  std::vector<ItemStack> stacks;
  stacks.reserve(slots.size());
  for(slot::Slot* currentSlot : slots) {
   stacks.push_back(currentSlot != nullptr ? currentSlot->getStack() : ItemStack{});
  }
  return stacks;
 }
 ItemStack onSlotClick(int index, int button, bool shift, PlayerEntity* player);
 [[nodiscard]] std::uint16_t nextRevision() {
  revision_ = static_cast<std::uint16_t>(revision_ + 1U);
  return revision_;
 }
 [[nodiscard]] slot::Slot* getSlot(int index) const {
  if(!hasSlot(index)) {
   return nullptr;
  }
  return slots[static_cast<std::size_t>(index)];
 }
 [[nodiscard]] slot::Slot* getSlot(Inventory* inventory, int index) const {
  for(slot::Slot* currentSlot : slots) {
   if(currentSlot != nullptr && currentSlot->equals(inventory, index)) {
    return currentSlot;
   }
  }
  return nullptr;
 }
 void setStackInSlot(int index, ItemStack stack) {
  if(slot::Slot* currentSlot = getSlot(index); currentSlot != nullptr) {
   currentSlot->setStack(std::move(stack));
  }
 }
 void updateSlotStacks(const std::vector<ItemStack>& stacks) {
  for(std::size_t i = 0; i < stacks.size(); ++i) {
   if(i >= slots.size()) {
    break;
   }
   if(slots[i] != nullptr) {
    slots[i]->setStack(stacks[i]);
   }
  }
 }
 virtual void setProperty(int /*id*/, int /*value*/) {
 }
 virtual void onAcknowledgementAccepted(std::uint16_t /*actionType*/) {
 }
 virtual void onAcknowledgementDenied(std::uint16_t /*actionType*/) {
 }
 [[nodiscard]] bool canOpen(PlayerEntity* player) const {
  return players_.find(player) == players_.end();
 }
 void updatePlayerList(PlayerEntity* player, bool remove) {
  if(player == nullptr) {
   return;
  }
  if(remove) {
   players_.erase(player);
  } else {
   players_.insert(player);
  }
 }

 private:
 [[nodiscard]] bool isOutputSlot(int slotIndex) const {
  return std::find(outputSlots_.begin(), outputSlots_.end(), slotIndex) != outputSlots_.end();
 }
 std::uint16_t revision_ = 0;
 std::unordered_set<PlayerEntity*> players_;
 int playerInventoryStart_ = -1;
 bool storageLayout_ = false;
 std::vector<int> outputSlots_;
};
} // namespace net::minecraft::screen
