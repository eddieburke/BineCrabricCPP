#pragma once
#include "net/minecraft/item/ArmorItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class ChainBootsItem : public ArmorItem {
 public:
 static constexpr int kRawId = 49;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 ChainBootsItem();
};
} // namespace net::minecraft::item
