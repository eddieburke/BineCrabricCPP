#pragma once
#include "net/minecraft/item/AxeItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class WoodenAxeItem : public AxeItem {
public:
  static constexpr int kRawId = 15;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  WoodenAxeItem();
};
} // namespace net::minecraft::item
