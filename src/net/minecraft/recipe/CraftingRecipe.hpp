#pragma once

#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft {
class CraftingInventory;
}

namespace net::minecraft::recipe {

// Faithful port of net.minecraft.recipe.CraftingRecipe (beta 1.7.3).
class CraftingRecipe {
public:
    virtual ~CraftingRecipe() = default;

    [[nodiscard]] virtual bool matches(const CraftingInventory& inventory) const = 0;
    [[nodiscard]] virtual ItemStack craft(const CraftingInventory& inventory) const = 0;
    [[nodiscard]] virtual int getSize() const = 0;
    [[nodiscard]] virtual ItemStack getOutput() const = 0;
};

} // namespace net::minecraft::recipe
