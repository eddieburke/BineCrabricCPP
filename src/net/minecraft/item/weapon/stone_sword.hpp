#pragma once

#include "net/minecraft/item/SwordItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class StoneSwordItem : public SwordItem {
public:    static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    StoneSwordItem();
};

} // namespace net::minecraft::item
