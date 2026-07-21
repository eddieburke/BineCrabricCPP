#pragma once
#include "net/minecraft/item/ShovelItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class GoldenShovelItem : public ShovelItem {
 public:
 static constexpr int kRawId = 28;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 GoldenShovelItem();
};
} // namespace net::minecraft::item
