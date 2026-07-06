#pragma once
#include "net/minecraft/item/FoodItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class RawFishItem : public FoodItem {
public:
  static constexpr int kRawId = 93;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  RawFishItem();
};
} // namespace net::minecraft::item
