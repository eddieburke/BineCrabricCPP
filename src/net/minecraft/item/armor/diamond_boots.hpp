#pragma once
#include "net/minecraft/item/ArmorItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class DiamondBootsItem : public ArmorItem {
public:
  static constexpr int kRawId = 57;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  DiamondBootsItem();
};
} // namespace net::minecraft::item
