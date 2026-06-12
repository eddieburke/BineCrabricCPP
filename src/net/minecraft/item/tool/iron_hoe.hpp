#pragma once

#include "net/minecraft/item/HoeItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class IronHoeItem : public HoeItem {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 36;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    IronHoeItem();
};

} // namespace net::minecraft::item
