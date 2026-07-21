#pragma once
#include "net/minecraft/item/ArmorItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class GoldenBootsItem : public ArmorItem {
 public:
 static constexpr int kRawId = 61;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 GoldenBootsItem();
};
} // namespace net::minecraft::item
