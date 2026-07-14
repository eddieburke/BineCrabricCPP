#pragma once
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class GunpowderItem : public Item {
public:
  static constexpr int kRawId = 33;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  GunpowderItem();
};
} // namespace net::minecraft::item
