#pragma once
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class SugarItem : public Item {
public:
  static constexpr int kRawId = 97;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  SugarItem();
};
} // namespace net::minecraft::item
