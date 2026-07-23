#pragma once
#include "net/minecraft/item/MinecartItem.hpp"
namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe
namespace net::minecraft::item {
class ChestMinecartItem : public MinecartItem {
 public:
 static constexpr int kRawId = 86;
 static void registerClass();
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
 ChestMinecartItem();
};
} // namespace net::minecraft::item
