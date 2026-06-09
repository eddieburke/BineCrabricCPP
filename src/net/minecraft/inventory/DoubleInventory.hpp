#pragma once

#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"

#include <memory>
#include <string>
#include <utility>

namespace net::minecraft {

class DoubleInventory : public Inventory {
public:
    DoubleInventory(std::string nameIn, Inventory* firstIn, Inventory* secondIn)
        : name_(std::move(nameIn)),
          first_(firstIn),
          second_(secondIn)
    {
    }

    [[nodiscard]] std::size_t size() const override
    {
        const std::size_t firstSize = first_ != nullptr ? first_->size() : 0;
        const std::size_t secondSize = second_ != nullptr ? second_->size() : 0;
        return firstSize + secondSize;
    }

    [[nodiscard]] std::string getName() const override
    {
        return name_;
    }

    [[nodiscard]] ItemStack getStack(std::size_t slot) const override
    {
        const std::size_t firstSize = first_ != nullptr ? first_->size() : 0;
        if (slot >= firstSize) {
            return second_ != nullptr ? second_->getStack(slot - firstSize) : ItemStack {};
        }
        return first_ != nullptr ? first_->getStack(slot) : ItemStack {};
    }

    [[nodiscard]] ItemStack removeStack(std::size_t slot, int amount) override
    {
        const std::size_t firstSize = first_ != nullptr ? first_->size() : 0;
        if (slot >= firstSize) {
            return second_ != nullptr ? second_->removeStack(slot - firstSize, amount) : ItemStack {};
        }
        return first_ != nullptr ? first_->removeStack(slot, amount) : ItemStack {};
    }

    void setStack(std::size_t slot, ItemStack stack) override
    {
        const std::size_t firstSize = first_ != nullptr ? first_->size() : 0;
        if (slot >= firstSize) {
            if (second_ != nullptr) {
                second_->setStack(slot - firstSize, stack);
            }
        } else {
            if (first_ != nullptr) {
                first_->setStack(slot, stack);
            }
        }
    }

    [[nodiscard]] int getMaxCountPerStack() const override
    {
        if (first_ != nullptr) {
            return first_->getMaxCountPerStack();
        }
        return second_ != nullptr ? second_->getMaxCountPerStack() : 64;
    }

    void markDirty() override
    {
        if (first_ != nullptr) {
            first_->markDirty();
        }
        if (second_ != nullptr) {
            second_->markDirty();
        }
    }

    [[nodiscard]] bool canPlayerUse(PlayerEntity* player) const override
    {
        return (first_ == nullptr || first_->canPlayerUse(player))
            && (second_ == nullptr || second_->canPlayerUse(player));
    }

private:
    std::string name_;
    Inventory* first_ = nullptr;
    Inventory* second_ = nullptr;
};

} // namespace net::minecraft
