#pragma once
#include "net/minecraft/item/ShovelItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class DiamondShovelItem : public ShovelItem {
   public:
    static constexpr int kRawId = 21;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    DiamondShovelItem();
};
}  // namespace net::minecraft::item
