#pragma once
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class FlintItem : public Item {
public:
  static constexpr int kRawId = 62;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  FlintItem();
};
} // namespace net::minecraft::item
