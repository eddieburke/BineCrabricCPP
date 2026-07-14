#pragma once
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class LeatherItem : public Item {
public:
  static constexpr int kRawId = 78;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  LeatherItem();
};
} // namespace net::minecraft::item
