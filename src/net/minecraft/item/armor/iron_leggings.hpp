#pragma once
#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
}  // namespace net::minecraft::recipe

namespace net::minecraft::item {
class IronLeggingsItem : public ArmorItem {
   public:
    static constexpr int kRawId = 52;
    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    IronLeggingsItem();
};
}  // namespace net::minecraft::item
