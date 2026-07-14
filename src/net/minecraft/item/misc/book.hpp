#pragma once
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class BookItem : public Item {
public:
  static constexpr int kRawId = 84;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  BookItem();
};
} // namespace net::minecraft::item
