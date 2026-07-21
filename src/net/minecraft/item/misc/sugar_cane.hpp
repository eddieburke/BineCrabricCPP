#pragma once
#include "net/minecraft/item/SecondaryBlockItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class SugarCaneItem : public SecondaryBlockItem {
 public:
 static constexpr int kRawId = 82;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 SugarCaneItem();
};
} // namespace net::minecraft::item
