#pragma once

#include "net/minecraft/item/HoeItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class IronHoeItem : public HoeItem {
public:    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    IronHoeItem();
};

} // namespace net::minecraft::item
