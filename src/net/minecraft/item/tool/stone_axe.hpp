#pragma once
#include "net/minecraft/item/AxeItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class StoneAxeItem : public AxeItem {
public:
  static constexpr int kRawId = 19;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  StoneAxeItem();
};
} // namespace net::minecraft::item
