#pragma once
#include "net/minecraft/item/ArmorItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class ChainHelmetItem : public ArmorItem {
 public:
 static constexpr int kRawId = 46;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 ChainHelmetItem();
};
} // namespace net::minecraft::item
