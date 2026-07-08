#pragma once
#include <vector>

#include "net/minecraft/inventory/CraftingInventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipe.hpp"

namespace net::minecraft::recipe {
// Faithful port of net.minecraft.recipe.ShapelessRecipe (beta 1.7.3).
class ShapelessRecipe : public CraftingRecipe {
   public:
    ShapelessRecipe(ItemStack output, std::vector<ItemStack> input) : output(output), input(std::move(input)) {
    }

    [[nodiscard]] ItemStack getOutput() const override {
        return this->output;
    }

    [[nodiscard]] bool matches(const CraftingInventory& craftingInventory) const override {
        std::vector<ItemStack> arrayList(this->input);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                ItemStack itemStack = craftingInventory.getStack(j, i);
                if (itemStack.empty()) {
                    continue;
                }
                bool bl = false;
                for (auto it = arrayList.begin(); it != arrayList.end(); ++it) {
                    const ItemStack& itemStack2 = *it;
                    if (itemStack.itemId != itemStack2.itemId ||
                        (itemStack2.getDamage() != -1 && itemStack.getDamage() != itemStack2.getDamage())) {
                        continue;
                    }
                    bl = true;
                    arrayList.erase(it);
                    break;
                }
                if (bl) {
                    continue;
                }
                return false;
            }
        }
        return arrayList.empty();
    }

    [[nodiscard]] ItemStack craft(const CraftingInventory& /*craftingInventory*/) const override {
        return this->output.copy();
    }

    [[nodiscard]] int getSize() const override {
        return static_cast<int>(this->input.size());
    }

   private:
    ItemStack output;
    std::vector<ItemStack> input;
};
}  // namespace net::minecraft::recipe
