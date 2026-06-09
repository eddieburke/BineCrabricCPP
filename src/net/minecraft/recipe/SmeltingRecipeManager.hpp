#pragma once

#include "net/minecraft/item/ItemStack.hpp"

#include <optional>
#include <unordered_map>

namespace net::minecraft::recipe {

// Faithful port of net.minecraft.recipe.SmeltingRecipeManager (beta 1.7.3).
class SmeltingRecipeManager {
public:
    [[nodiscard]] static SmeltingRecipeManager& instance();

    void addRecipe(int inputId, ItemStack output);
    [[nodiscard]] std::optional<ItemStack> craft(int inputId) const;
    [[nodiscard]] const std::unordered_map<int, ItemStack>& getRecipes() const { return recipes_; }

private:
    SmeltingRecipeManager();

    std::unordered_map<int, ItemStack> recipes_;
};

} // namespace net::minecraft::recipe
