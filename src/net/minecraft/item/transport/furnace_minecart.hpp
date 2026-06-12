#pragma once

#include "net/minecraft/item/MinecartItem.hpp"

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class FurnaceMinecartItem : public MinecartItem {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 87;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

    FurnaceMinecartItem();
};

} // namespace net::minecraft::item
