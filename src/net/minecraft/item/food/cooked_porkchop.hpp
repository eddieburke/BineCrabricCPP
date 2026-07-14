#pragma once
#include "net/minecraft/item/FoodItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class CookedPorkchopItem : public FoodItem {
public:
  static constexpr int kRawId = 64;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  static void registerSmeltingRecipes();
  CookedPorkchopItem();
};
} // namespace net::minecraft::item
