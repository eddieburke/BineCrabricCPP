#pragma once

#include "net/minecraft/item/AxeItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class IronAxeItem : public AxeItem {
public:
    static constexpr int kRawId = 2;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    IronAxeItem();
};

} // namespace net::minecraft::item
