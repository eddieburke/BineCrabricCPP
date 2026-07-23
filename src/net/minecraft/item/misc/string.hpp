#pragma once
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class StringItem : public Item {
 public:
 static constexpr int kRawId = 31;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 StringItem();
};
} // namespace net::minecraft::item
