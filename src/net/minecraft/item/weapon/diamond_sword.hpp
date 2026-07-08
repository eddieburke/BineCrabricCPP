#pragma once
#include "net/minecraft/item/SwordItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class DiamondSwordItem : public SwordItem {
   public:
    static constexpr int kRawId = 20;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    DiamondSwordItem();
};
}  // namespace net::minecraft::item
