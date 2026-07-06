#pragma once
#include "net/minecraft/item/PickaxeItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class DiamondPickaxeItem : public PickaxeItem {
public:
  static constexpr int kRawId = 22;
  static void registerClass();
  static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
  DiamondPickaxeItem();
};
} // namespace net::minecraft::item
