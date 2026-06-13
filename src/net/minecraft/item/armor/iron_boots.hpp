#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class IronBootsItem : public ArmorItem {
public:
    static constexpr int kRawId = 53;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    IronBootsItem();
};

} // namespace net::minecraft::item
