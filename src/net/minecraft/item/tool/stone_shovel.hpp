#pragma once
#include "net/minecraft/item/ShovelItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class StoneShovelItem : public ShovelItem {
 public:
 static constexpr int kRawId = 17;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 StoneShovelItem();
};
} // namespace net::minecraft::item
