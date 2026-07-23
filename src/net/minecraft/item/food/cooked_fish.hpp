#pragma once
#include "net/minecraft/item/FoodItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class CookedFishItem : public FoodItem {
 public:
 static constexpr int kRawId = 94;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 static void registerSmeltingRecipes();
 CookedFishItem();
};
} // namespace net::minecraft::item
