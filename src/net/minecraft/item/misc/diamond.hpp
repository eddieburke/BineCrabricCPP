#pragma once
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class DiamondItem : public Item {
   public:
    static constexpr int kRawId = 8;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    static void registerSmeltingRecipes();
    DiamondItem();
};
}  // namespace net::minecraft::item
