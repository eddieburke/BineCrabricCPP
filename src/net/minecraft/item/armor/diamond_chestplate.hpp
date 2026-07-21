#pragma once
#include "net/minecraft/item/ArmorItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class DiamondChestplateItem : public ArmorItem {
 public:
 static constexpr int kRawId = 55;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 DiamondChestplateItem();
};
} // namespace net::minecraft::item
