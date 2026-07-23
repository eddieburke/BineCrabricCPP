#pragma once
#include "net/minecraft/item/AxeItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class GoldenAxeItem : public AxeItem {
 public:
 static constexpr int kRawId = 30;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 GoldenAxeItem();
};
} // namespace net::minecraft::item
