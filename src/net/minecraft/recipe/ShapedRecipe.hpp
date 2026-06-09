#pragma once

#include "net/minecraft/inventory/CraftingInventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipe.hpp"

#include <vector>

namespace net::minecraft::recipe {

// Faithful port of net.minecraft.recipe.ShapedRecipe (beta 1.7.3).
// An empty() ItemStack in `input` represents Java's null pattern cell.
class ShapedRecipe : public CraftingRecipe {
public:
    const int outputId;

    ShapedRecipe(int width, int height, std::vector<ItemStack> input, ItemStack output)
        : outputId(output.itemId),
          width(width),
          height(height),
          input(std::move(input)),
          output(output) {}

    [[nodiscard]] ItemStack getOutput() const override { return this->output; }

    [[nodiscard]] bool matches(const CraftingInventory& craftingInventory) const override
    {
        for (int i = 0; i <= 3 - this->width; ++i) {
            for (int j = 0; j <= 3 - this->height; ++j) {
                if (this->matchesPattern(craftingInventory, i, j, true)) {
                    return true;
                }
                if (!this->matchesPattern(craftingInventory, i, j, false)) {
                    continue;
                }
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] ItemStack craft(const CraftingInventory& /*craftingInventory*/) const override
    {
        return ItemStack(this->output.itemId, this->output.count, this->output.getDamage());
    }

    [[nodiscard]] int getSize() const override { return this->width * this->height; }

private:
    int width;
    int height;
    std::vector<ItemStack> input;
    ItemStack output;

    [[nodiscard]] bool matchesPattern(const CraftingInventory& inv, int offsetX, int offsetY, bool flipped) const
    {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                int n = i - offsetX;
                int n2 = j - offsetY;
                ItemStack itemStack2{}; // null cell
                if (n >= 0 && n2 >= 0 && n < this->width && n2 < this->height) {
                    itemStack2 = flipped ? this->input[this->width - n - 1 + n2 * this->width]
                                         : this->input[n + n2 * this->width];
                }
                ItemStack itemStack = inv.getStack(i, j);
                const bool stackNull = itemStack.empty();
                const bool patternNull = itemStack2.empty();
                if (stackNull && patternNull) {
                    continue;
                }
                if ((stackNull && !patternNull) || (!stackNull && patternNull)) {
                    return false;
                }
                if (itemStack2.itemId != itemStack.itemId) {
                    return false;
                }
                if (itemStack2.getDamage() == -1 || itemStack2.getDamage() == itemStack.getDamage()) {
                    continue;
                }
                return false;
            }
        }
        return true;
    }
};

} // namespace net::minecraft::recipe
