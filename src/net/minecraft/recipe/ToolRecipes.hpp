#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include <string>
#include <vector>

namespace net::minecraft::recipe {

// Faithful port of net.minecraft.recipe.ToolRecipes (beta 1.7.3).
class ToolRecipes {
public:
    void add(CraftingRecipeManager& recipeManager)
    {
        for (std::size_t i = 0; i < items[0].size(); ++i) {
            RecipeArg object = items[0][i];
            for (std::size_t j = 0; j + 1 < items.size(); ++j) {
                Item* item = items[j + 1][i].item();
                recipeManager.addShapedRecipe(ItemStack(item),
                    {patterns[j], '#', Item::STICK, 'X', object});
            }
        }
        recipeManager.addShapedRecipe(ItemStack(Item::SHEARS),
            {std::string(" #"), std::string("# "), '#', Item::IRON_INGOT});
    }

private:
    std::vector<std::vector<std::string>> patterns{
        {"XXX", " # ", " # "}, {"X", "#", "#"}, {"XX", "X#", " #"}, {"XX", " #", " #"}};
    std::vector<std::vector<RecipeArg>> items{
        {Block::PLANKS, Block::COBBLESTONE, Item::IRON_INGOT, Item::DIAMOND, Item::GOLD_INGOT},
        {Item::WOODEN_PICKAXE, Item::STONE_PICKAXE, Item::IRON_PICKAXE, Item::DIAMOND_PICKAXE, Item::GOLDEN_PICKAXE},
        {Item::WOODEN_SHOVEL, Item::STONE_SHOVEL, Item::IRON_SHOVEL, Item::DIAMOND_SHOVEL, Item::GOLDEN_SHOVEL},
        {Item::WOODEN_AXE, Item::STONE_AXE, Item::IRON_AXE, Item::DIAMOND_AXE, Item::GOLDEN_AXE},
        {Item::WOODEN_HOE, Item::STONE_HOE, Item::IRON_HOE, Item::DIAMOND_HOE, Item::GOLDEN_HOE}};
};

} // namespace net::minecraft::recipe
