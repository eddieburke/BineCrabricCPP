#pragma once

#include "net/minecraft/item/PickaxeItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class WoodenPickaxeItem : public PickaxeItem {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 14;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    WoodenPickaxeItem();
};

} // namespace net::minecraft::item
