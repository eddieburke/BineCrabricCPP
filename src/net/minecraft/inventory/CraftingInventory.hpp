#pragma once

#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"

#include <cstddef>
#include <vector>

namespace net::minecraft::screen {
class ScreenHandler;
}

namespace net::minecraft {

class CraftingInventory : public Inventory {
public:
    CraftingInventory(screen::ScreenHandler* handler, int widthIn, int heightIn)
        : handler_(handler),
          width_(widthIn),
          stacks_(static_cast<std::size_t>(widthIn * heightIn))
    {
    }

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

    [[nodiscard]] ItemStack getStack(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= width_) {
            return {};
        }
        return getStack(static_cast<std::size_t>(x + y * width_));
    }

    [[nodiscard]] std::string getName() const override
    {
        return "Crafting";
    }

    [[nodiscard]] ItemStack removeStack(std::size_t slot, int amount) override
    {
        if (slot >= stacks_.size() || stacks_[slot].empty()) {
            return {};
        }
        if (stacks_[slot].count <= amount) {
            ItemStack removed = stacks_[slot];
            stacks_[slot] = {};
            notifySlotUpdate();
            return removed;
        }
        ItemStack removed = stacks_[slot].split(amount);
        if (stacks_[slot].count == 0) {
            stacks_[slot] = {};
        }
        notifySlotUpdate();
        return removed;
    }

    void setStack(std::size_t slot, ItemStack stack) override
    {
        if (slot >= stacks_.size()) {
            return;
        }
        stacks_[slot] = stack;
        notifySlotUpdate();
    }

    [[nodiscard]] int getMaxCountPerStack() const override
    {
        return 64;
    }

    void markDirty() override {}

    [[nodiscard]] bool canPlayerUse(PlayerEntity* player) const override
    {
        (void)player;
        return true;
    }

private:
    void notifySlotUpdate();

    screen::ScreenHandler* handler_ = nullptr;
    int width_ = 0;
    std::vector<ItemStack> stacks_;
};

} // namespace net::minecraft
