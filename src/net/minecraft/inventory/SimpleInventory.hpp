#pragma once

#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/inventory/InventoryListener.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace net::minecraft {

class SimpleInventory : public Inventory {
public:
    SimpleInventory(std::string name, std::size_t size)
        : name_(std::move(name)),
          stacks_(size)
    {
    }

    [[nodiscard]] ItemStack getStack(std::size_t slot) const override
    {
        if (slot >= stacks_.size()) {
            return {};
        }
        return stacks_[slot];
    }

    [[nodiscard]] ItemStack removeStack(std::size_t slot, int amount) override
    {
        if (slot >= stacks_.size()) {
            return {};
        }
        ItemStack& stack = stacks_[slot];
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

    void setStack(std::size_t slot, ItemStack stack) override
    {
        if (slot >= stacks_.size()) {
            return;
        }
        if (!stack.empty() && stack.count > getMaxCountPerStack()) {
            stack.count = getMaxCountPerStack();
        }
        stacks_[slot] = stack;
        markDirty();
    }

    [[nodiscard]] std::size_t size() const override
    {
        return stacks_.size();
    }

    [[nodiscard]] std::string getName() const override
    {
        return name_;
    }

    [[nodiscard]] int getMaxCountPerStack() const override
    {
        return 64;
    }

    void markDirty() override
    {
        for (InventoryListener* listener : listeners_) {
            if (listener != nullptr) {
                listener->onInventoryChanged(this);
            }
        }
    }

    [[nodiscard]] bool canPlayerUse(PlayerEntity* player) const override
    {
        (void)player;
        return true;
    }

    void addListener(InventoryListener* listener)
    {
        listeners_.push_back(listener);
    }

private:
    std::string name_;
    std::vector<ItemStack> stacks_;
    std::vector<InventoryListener*> listeners_;
};

} // namespace net::minecraft
