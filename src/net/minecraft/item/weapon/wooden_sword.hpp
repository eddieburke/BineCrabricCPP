#pragma once
#include "net/minecraft/item/SwordItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class WoodenSwordItem : public SwordItem {
public:
  static constexpr int kRawId = 12;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  WoodenSwordItem();
};
} // namespace net::minecraft::item
