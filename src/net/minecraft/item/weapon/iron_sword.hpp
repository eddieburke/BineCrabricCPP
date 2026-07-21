#pragma once
#include "net/minecraft/item/SwordItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class IronSwordItem : public SwordItem {
 public:
 static constexpr int kRawId = 11;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 IronSwordItem();
};
} // namespace net::minecraft::item
