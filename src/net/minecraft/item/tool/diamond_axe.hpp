#pragma once

#include "net/minecraft/item/AxeItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class DiamondAxeItem : public AxeItem {
public:    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    DiamondAxeItem();
};

} // namespace net::minecraft::item
