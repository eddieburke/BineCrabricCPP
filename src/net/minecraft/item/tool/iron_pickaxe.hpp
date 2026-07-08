#pragma once
#include "net/minecraft/item/PickaxeItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class IronPickaxeItem : public PickaxeItem {
   public:
    static constexpr int kRawId = 1;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    IronPickaxeItem();
};
}  // namespace net::minecraft::item
