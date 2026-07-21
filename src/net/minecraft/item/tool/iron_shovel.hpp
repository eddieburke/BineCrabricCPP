#pragma once
#include "net/minecraft/item/ShovelItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class IronShovelItem : public ShovelItem {
 public:
 static constexpr int kRawId = 0;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 IronShovelItem();
};
} // namespace net::minecraft::item
