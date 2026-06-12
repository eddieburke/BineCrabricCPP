#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft

namespace net::minecraft::recipe {
class CraftingRecipeManager;
} // namespace net::minecraft::recipe

namespace net::minecraft::item {

class BowItem : public Item {
public:        static constexpr bool kRegisters = true;
    static constexpr int kRawId = 5;

static void registerClass();
    static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
    BowItem();
    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override;
};

} // namespace net::minecraft::item
