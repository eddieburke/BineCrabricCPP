#pragma once
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class BoneItem : public Item {
 public:
 static constexpr int kRawId = 96;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 BoneItem();
};
} // namespace net::minecraft::item
