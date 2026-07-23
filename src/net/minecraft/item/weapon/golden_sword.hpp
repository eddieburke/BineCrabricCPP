#pragma once
#include "net/minecraft/item/SwordItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class GoldenSwordItem : public SwordItem {
 public:
 static constexpr int kRawId = 27;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 GoldenSwordItem();
};
} // namespace net::minecraft::item
