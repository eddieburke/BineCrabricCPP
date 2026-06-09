#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include <string>
#include <vector>

namespace net::minecraft::recipe {

class WeaponRecipes {
public:
    void add(CraftingRecipeManager& recipeManager)
    {
        for (std::size_t i = 0; i < items[0].size(); ++i) {
            RecipeArg material = items[0][i];
            for (std::size_t j = 0; j + 1 < items.size(); ++j) {
                Item* item = items[j + 1][i].item();
                recipeManager.addShapedRecipe(ItemStack(item),
                    {patterns[j], '#', Item::STICK, 'X', material});
            }
        }
        recipeManager.addShapedRecipe(ItemStack(Item::BOW, 1),
            {std::string(" #X"), std::string("# X"), std::string(" #X"), 'X', Item::STRING, '#', Item::STICK});
        recipeManager.addShapedRecipe(ItemStack(Item::ARROW, 4),
            {std::string("X"), std::string("#"), std::string("Y"), 'Y', Item::FEATHER, 'X', Item::FLINT, '#', Item::STICK});
    }

private:
    std::vector<std::vector<std::string>> patterns{{"X", "X", "#"}};
    std::vector<std::vector<RecipeArg>> items{
        {Block::PLANKS, Block::COBBLESTONE, Item::IRON_INGOT, Item::DIAMOND, Item::GOLD_INGOT},
        {Item::WOODEN_SWORD, Item::STONE_SWORD, Item::IRON_SWORD, Item::DIAMOND_SWORD, Item::GOLDEN_SWORD}};
};

} // namespace net::minecraft::recipe
