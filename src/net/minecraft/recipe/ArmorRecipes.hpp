#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include <string>
#include <vector>

namespace net::minecraft::recipe {

class ArmorRecipes {
public:
    void add(CraftingRecipeManager& recipeManager)
    {
        for (std::size_t i = 0; i < items[0].size(); ++i) {
            RecipeArg material = items[0][i];
            for (std::size_t j = 0; j + 1 < items.size(); ++j) {
                Item* item = items[j + 1][i].item();
                recipeManager.addShapedRecipe(ItemStack(item), {patterns[j], 'X', material});
            }
        }
    }

private:
    std::vector<std::vector<std::string>> patterns{
        {"XXX", "X X"}, {"X X", "XXX", "XXX"}, {"XXX", "X X", "X X"}, {"X X", "X X"}};
    std::vector<std::vector<RecipeArg>> items{
        {Item::LEATHER, Block::FIRE, Item::IRON_INGOT, Item::DIAMOND, Item::GOLD_INGOT},
        {Item::LEATHER_HELMET, Item::CHAIN_HELMET, Item::IRON_HELMET, Item::DIAMOND_HELMET, Item::GOLDEN_HELMET},
        {Item::LEATHER_CHESTPLATE, Item::CHAIN_CHESTPLATE, Item::IRON_CHESTPLATE, Item::DIAMOND_CHESTPLATE, Item::GOLDEN_CHESTPLATE},
        {Item::LEATHER_LEGGINGS, Item::CHAIN_LEGGINGS, Item::IRON_LEGGINGS, Item::DIAMOND_LEGGINGS, Item::GOLDEN_LEGGINGS},
        {Item::LEATHER_BOOTS, Item::CHAIN_BOOTS, Item::IRON_BOOTS, Item::DIAMOND_BOOTS, Item::GOLDEN_BOOTS}};
};

} // namespace net::minecraft::recipe
