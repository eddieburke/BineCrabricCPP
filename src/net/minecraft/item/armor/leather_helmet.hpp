#pragma once
#include "net/minecraft/item/ArmorItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class LeatherHelmetItem : public ArmorItem {
 public:
 static constexpr int kRawId = 42;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 LeatherHelmetItem();
};
} // namespace net::minecraft::item
