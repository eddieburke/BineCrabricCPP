#pragma once
#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class GoldenAppleItem : public FoodItem {
   public:
    static constexpr int kRawId = 66;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    GoldenAppleItem();
};
}  // namespace net::minecraft::item
