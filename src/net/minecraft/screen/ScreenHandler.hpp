#pragma once

#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/screen/ScreenHandlerListener.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace net::minecraft::screen {

class ScreenHandler {
public:
    virtual ~ScreenHandler() = default;

    std::vector<ItemStack> trackedStacks;
    std::vector<slot::Slot*> slots;
    int syncId = 0;

protected:
    void addSlot(slot::Slot* slot)
    {
        slot->id = static_cast<int>(slots.size());
        slots.push_back(slot);
        trackedStacks.emplace_back();
    }

    [[nodiscard]] bool hasSlot(int index) const
    {
        return index >= 0 && index < static_cast<int>(slots.size());
    }

    [[nodiscard]] virtual ItemStack quickMove(int slotIndex)
    {
        if (!hasSlot(slotIndex)) {
            return {};
        }
        slot::Slot* currentSlot = slots[static_cast<std::size_t>(slotIndex)];
        if (currentSlot == nullptr) {
            return {};
        }
        return currentSlot->getStack();
    }

    void insertItem(ItemStack& stack, int startIndex, int endIndex, bool fromLast)
    {
        if (stack.empty() || startIndex < 0 || endIndex < 0 || startIndex > endIndex
            || endIndex > static_cast<int>(slots.size())) {
            return;
        }
        int slotIndex = startIndex;
        if (fromLast) {
            slotIndex = endIndex - 1;
        }
        if (stack.isStackable()) {
            while (stack.count > 0
                && ((!fromLast && slotIndex < endIndex) || (fromLast && slotIndex >= startIndex))) {
                slot::Slot* currentSlot = getSlot(slotIndex);
                if (currentSlot != nullptr) {
                    ItemStack slotStack = currentSlot->getStack();
                    if (!slotStack.empty() && slotStack.itemId == stack.itemId
                        && (!stack.hasSubtypes() || slotStack.getDamage() == stack.getDamage())) {
                        int combined = slotStack.count + stack.count;
                        if (combined <= stack.getMaxCount()) {
                            stack.count = 0;
                            slotStack.count = combined;
                            currentSlot->setStack(slotStack);
                        } else if (slotStack.count < stack.getMaxCount()) {
                            stack.count -= stack.getMaxCount() - slotStack.count;
                            slotStack.count = stack.getMaxCount();
                            currentSlot->setStack(slotStack);
                        }
                    }
                }
                if (fromLast) {
                    --slotIndex;
                } else {
                    ++slotIndex;
                }
            }
        }
        if (stack.count > 0) {
            slotIndex = fromLast ? endIndex - 1 : startIndex;
            while ((!fromLast && slotIndex < endIndex) || (fromLast && slotIndex >= startIndex)) {
                slot::Slot* currentSlot = getSlot(slotIndex);
                if (currentSlot != nullptr) {
                    ItemStack slotStack = currentSlot->getStack();
                    if (slotStack.empty()) {
                        currentSlot->setStack(stack.copy());
                        stack.count = 0;
                        break;
                    }
                }
                if (fromLast) {
                    --slotIndex;
                } else {
                    ++slotIndex;
                }
            }
        }
    }

public:
    [[nodiscard]] virtual bool canUse(PlayerEntity* /*player*/) { return true; }

    virtual void sendContentUpdates()
    {
        for (std::size_t i = 0; i < slots.size(); ++i) {
            slot::Slot* currentSlot = slots[i];
            if (currentSlot == nullptr) {
                continue;
            }
            ItemStack stack = currentSlot->getStack();
            if (ItemStack::areEqual(trackedStacks[i], stack)) {
                continue;
            }
            trackedStacks[i] = stack.empty() ? ItemStack{} : stack.copy();
            for (ScreenHandlerListener* listener : listeners_) {
                if (listener != nullptr) {
                    listener->onSlotUpdate(this, static_cast<int>(i), trackedStacks[i]);
                }
            }
        }
    }

    virtual void onClosed(PlayerEntity* player);
    virtual void onSlotUpdate(Inventory* /*inventory*/)
    {
        sendContentUpdates();
    }

    void addListener(ScreenHandlerListener* listener)
    {
        if (listener == nullptr) {
            return;
        }
        for (ScreenHandlerListener* existing : listeners_) {
            if (existing == listener) {
                throw std::invalid_argument("Listener already listening");
            }
        }
        listeners_.push_back(listener);
        listener->onContentsUpdate(this, getStacks());
        sendContentUpdates();
    }

    [[nodiscard]] std::vector<ItemStack> getStacks() const
    {
        std::vector<ItemStack> stacks;
        stacks.reserve(slots.size());
        for (slot::Slot* currentSlot : slots) {
            stacks.push_back(currentSlot != nullptr ? currentSlot->getStack() : ItemStack{});
        }
        return stacks;
    }

    ItemStack onSlotClick(int index, int button, bool shift, PlayerEntity* player);
    [[nodiscard]] std::uint16_t nextRevision()
    {
        revision_ = static_cast<std::uint16_t>(revision_ + 1U);
        return revision_;
    }
    [[nodiscard]] slot::Slot* getSlot(int index) const
    {
        if (!hasSlot(index)) {
            return nullptr;
        }
        return slots[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] slot::Slot* getSlot(Inventory* inventory, int index) const
    {
        for (slot::Slot* currentSlot : slots) {
            if (currentSlot != nullptr && currentSlot->equals(inventory, index)) {
                return currentSlot;
            }
        }
        return nullptr;
    }

    void setStackInSlot(int index, ItemStack stack)
    {
        if (slot::Slot* currentSlot = getSlot(index); currentSlot != nullptr) {
            currentSlot->setStack(std::move(stack));
        }
    }

    void updateSlotStacks(const std::vector<ItemStack>& stacks)
    {
        for (std::size_t i = 0; i < stacks.size(); ++i) {
            if (i >= slots.size()) {
                break;
            }
            if (slots[i] != nullptr) {
                slots[i]->setStack(stacks[i]);
            }
        }
    }

    virtual void setProperty(int /*id*/, int /*value*/) {}

    virtual void onAcknowledgementAccepted(std::uint16_t /*actionType*/) {}

    virtual void onAcknowledgementDenied(std::uint16_t /*actionType*/) {}

    [[nodiscard]] bool canOpen(PlayerEntity* player) const
    {
        return players_.find(player) == players_.end();
    }

    void updatePlayerList(PlayerEntity* player, bool remove)
    {
        if (player == nullptr) {
            return;
        }
        if (remove) {
            players_.erase(player);
        } else {
            players_.insert(player);
        }
    }

    std::vector<ScreenHandlerListener*> listeners_;

private:
    std::uint16_t revision_ = 0;
    std::unordered_set<PlayerEntity*> players_;
};

} // namespace net::minecraft::screen
