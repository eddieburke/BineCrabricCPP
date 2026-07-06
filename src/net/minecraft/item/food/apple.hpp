#pragma once
#include "net/minecraft/item/FoodItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class AppleItem : public FoodItem {
public:
  static constexpr int kRawId = 4;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  AppleItem();
};
} // namespace net::minecraft::item
