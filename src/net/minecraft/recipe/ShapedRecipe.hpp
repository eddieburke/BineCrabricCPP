#pragma once
#include <vector>
#include "net/minecraft/inventory/CraftingInventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipe.hpp"
namespace net::minecraft::recipe {
// Faithful port of net.minecraft.recipe.ShapedRecipe (beta 1.7.3).
// An empty() ItemStack in `input` represents Java's null pattern cell.
class ShapedRecipe : public CraftingRecipe {
public:
  const int outputId;
  ShapedRecipe(int width, int height, std::vector<ItemStack> input, ItemStack output)
      : outputId(output.itemId), width(width), height(height), input(std::move(input)), output(output) {
  }
  [[nodiscard]] ItemStack getOutput() const override {
    return this->output;
  }
  [[nodiscard]] bool matches(const CraftingInventory& craftingInventory) const override {
    for(int i = 0; i <= 3 - this->width; ++i) {
      for(int j = 0; j <= 3 - this->height; ++j) {
        if(this->matchesPattern(craftingInventory, i, j, true)) {
          return true;
        }
        if(!this->matchesPattern(craftingInventory, i, j, false)) {
          continue;
        }
        return true;
      }
    }
    return false;
  }
  [[nodiscard]] ItemStack craft(const CraftingInventory& /*craftingInventory*/) const override {
    return ItemStack(this->output.itemId, this->output.count, this->output.getDamage());
  }
  [[nodiscard]] int getSize() const override {
    return this->width * this->height;
  }

private:
  int width;
  int height;
  std::vector<ItemStack> input;
  ItemStack output;
  [[nodiscard]] bool matchesPattern(const CraftingInventory& inv, int offsetX, int offsetY, bool flipped) const {
    for(int i = 0; i < 3; ++i) {
      for(int j = 0; j < 3; ++j) {
        int patternX = i - offsetX;
        int patternY = j - offsetY;
        ItemStack patternStack{}; // null cell
        if(patternX >= 0 && patternY >= 0 && patternX < this->width && patternY < this->height) {
          patternStack = flipped ? this->input[this->width - patternX - 1 + patternY * this->width]
                                 : this->input[patternX + patternY * this->width];
        }
        ItemStack stack = inv.getStack(i, j);
        const bool stackNull = stack.empty();
        const bool patternNull = patternStack.empty();
        if(stackNull && patternNull) {
          continue;
        }
        if((stackNull && !patternNull) || (!stackNull && patternNull)) {
          return false;
        }
        if(patternStack.itemId != stack.itemId) {
          return false;
        }
        if(patternStack.getDamage() == -1 || patternStack.getDamage() == stack.getDamage()) {
          continue;
        }
        return false;
      }
    }
    return true;
  }
};
} // namespace net::minecraft::recipe
