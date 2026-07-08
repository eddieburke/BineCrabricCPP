#pragma once
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class GoldIngotItem : public Item {
   public:
    static constexpr int kRawId = 10;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    static void registerSmeltingRecipes();
    GoldIngotItem();
};
}  // namespace net::minecraft::item
