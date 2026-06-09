#pragma once

#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"

#include <array>
#include <vector>

namespace net::minecraft {

class CraftingResultInventory : public Inventory {
public:
    [[nodiscard]] std::size_t size() const override
    {
        return stacks_.size();
    }

    [[nodiscard]] ItemStack getStack(std::size_t slot) const override
    {
        if (slot >= stacks_.size()) {
            return {};
        }
        return stacks_[slot];
    }

    [[nodiscard]] ItemStack removeStack(std::size_t slot, int /*amount*/) override
    {
        if (slot >= stacks_.size()) {
            return {};
        }
        if (stacks_[slot].empty()) {
            return {};
        }
        ItemStack removed = stacks_[slot];
        stacks_[slot] = {};
        return removed;
    }

    void setStack(std::size_t slot, ItemStack stack) override
    {
        if (slot >= stacks_.size()) {
            return;
        }
        stacks_[slot] = stack;
    }

    [[nodiscard]] std::string getName() const override
    {
        return "Result";
    }

    [[nodiscard]] int getMaxCountPerStack() const override
    {
        return 64;
    }

    void markDirty() override
    {
    }

    [[nodiscard]] bool canPlayerUse(PlayerEntity* player) const override
    {
        (void)player;
        return true;
    }

private:
    std::array<ItemStack, 1> stacks_ {};
};

} // namespace net::minecraft
