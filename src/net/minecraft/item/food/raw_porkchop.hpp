#pragma once
#include "net/minecraft/item/FoodItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class RawPorkchopItem : public FoodItem {
 public:
 static constexpr int kRawId = 63;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 RawPorkchopItem();
};
} // namespace net::minecraft::item
