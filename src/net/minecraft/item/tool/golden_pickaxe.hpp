#pragma once

#include "net/minecraft/item/PickaxeItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class GoldenPickaxeItem : public PickaxeItem {
public:    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    GoldenPickaxeItem();
};

} // namespace net::minecraft::item
