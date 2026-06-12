#pragma once

#include "net/minecraft/item/ShovelItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class WoodenShovelItem : public ShovelItem {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 13;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    WoodenShovelItem();
};

} // namespace net::minecraft::item
