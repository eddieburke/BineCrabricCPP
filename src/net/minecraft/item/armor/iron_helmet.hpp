#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class IronHelmetItem : public ArmorItem {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 50;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    IronHelmetItem();
};

} // namespace net::minecraft::item
