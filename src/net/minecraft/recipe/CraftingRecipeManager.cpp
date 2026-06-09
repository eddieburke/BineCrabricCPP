#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/inventory/CraftingInventory.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/ArmorRecipes.hpp"
#include "net/minecraft/recipe/BasicBlockRecipes.hpp"
#include "net/minecraft/recipe/DyeRecipes.hpp"
#include "net/minecraft/recipe/FoodRecipes.hpp"
#include "net/minecraft/recipe/ShapedRecipe.hpp"
#include "net/minecraft/recipe/ShapelessRecipe.hpp"
#include "net/minecraft/recipe/StorageBlockRecipes.hpp"
#include "net/minecraft/recipe/ToolRecipes.hpp"
#include "net/minecraft/recipe/WeaponRecipes.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace net::minecraft::recipe {

namespace {

void sortRecipes(std::vector<std::unique_ptr<CraftingRecipe>>& recipes)
{
    std::stable_sort(recipes.begin(), recipes.end(),
        [](const std::unique_ptr<CraftingRecipe>& left, const std::unique_ptr<CraftingRecipe>& right) {
            const bool leftShapeless = dynamic_cast<const ShapelessRecipe*>(left.get()) != nullptr;
            const bool rightShapeless = dynamic_cast<const ShapelessRecipe*>(right.get()) != nullptr;
            const bool leftShaped = dynamic_cast<const ShapedRecipe*>(left.get()) != nullptr;
            const bool rightShaped = dynamic_cast<const ShapedRecipe*>(right.get()) != nullptr;
            if (leftShapeless && rightShaped) {
                return false;
            }
            if (rightShapeless && leftShaped) {
                return true;
            }
            if (right->getSize() < left->getSize()) {
                return false;
            }
            if (right->getSize() > left->getSize()) {
                return true;
            }
            return false;
        });
}

} // namespace

CraftingRecipeManager& CraftingRecipeManager::getInstance()
{
    static CraftingRecipeManager instance;
    return instance;
}

CraftingRecipeManager::CraftingRecipeManager()
{
    ToolRecipes().add(*this);
    WeaponRecipes().add(*this);
    StorageBlockRecipes().add(*this);
    FoodRecipes().add(*this);
    BasicBlockRecipes().add(*this);
    ArmorRecipes().add(*this);
    DyeRecipes().add(*this);

    addShapedRecipe(ItemStack(Item::PAPER, 3), {std::string("###"), '#', Item::SUGAR_CANE});
    addShapedRecipe(ItemStack(Item::BOOK, 1), {std::string("#"), std::string("#"), std::string("#"), '#', Item::PAPER});
    addShapedRecipe(ItemStack(Block::FENCE, 2), {std::string("###"), std::string("###"), '#', Item::STICK});
    addShapedRecipe(ItemStack(Block::JUKEBOX, 1),
        {std::string("###"), std::string("#X#"), std::string("###"), '#', Block::PLANKS, 'X', Item::DIAMOND});
    addShapedRecipe(ItemStack(Block::NOTE_BLOCK, 1),
        {std::string("###"), std::string("#X#"), std::string("###"), '#', Block::PLANKS, 'X', Item::REDSTONE});
    addShapedRecipe(ItemStack(Block::BOOKSHELF, 1),
        {std::string("###"), std::string("XXX"), std::string("###"), '#', Block::PLANKS, 'X', Item::BOOK});
    addShapedRecipe(ItemStack(Block::SNOW_BLOCK, 1), {std::string("##"), std::string("##"), '#', Item::SNOWBALL});
    addShapedRecipe(ItemStack(Block::CLAY, 1), {std::string("##"), std::string("##"), '#', Item::CLAY});
    addShapedRecipe(ItemStack(Block::BRICKS, 1), {std::string("##"), std::string("##"), '#', Item::BRICK});
    addShapedRecipe(ItemStack(Block::GLOWSTONE, 1), {std::string("##"), std::string("##"), '#', Item::GLOWSTONE_DUST});
    addShapedRecipe(ItemStack(Block::WOOL, 1), {std::string("##"), std::string("##"), '#', Item::STRING});
    addShapedRecipe(ItemStack(Block::TNT, 1),
        {std::string("X#X"), std::string("#X#"), std::string("X#X"), 'X', Item::GUNPOWDER, '#', Block::SAND});
    addShapedRecipe(ItemStack(Block::SLAB, 3, 3), {std::string("###"), '#', Block::COBBLESTONE});
    addShapedRecipe(ItemStack(Block::SLAB, 3, 0), {std::string("###"), '#', Block::STONE});
    addShapedRecipe(ItemStack(Block::SLAB, 3, 1), {std::string("###"), '#', Block::SANDSTONE});
    addShapedRecipe(ItemStack(Block::SLAB, 3, 2), {std::string("###"), '#', Block::PLANKS});
    addShapedRecipe(ItemStack(Block::LADDER, 2),
        {std::string("# #"), std::string("###"), std::string("# #"), '#', Item::STICK});
    addShapedRecipe(ItemStack(Item::WOODEN_DOOR, 1), {std::string("##"), std::string("##"), std::string("##"), '#', Block::PLANKS});
    addShapedRecipe(ItemStack(Block::TRAPDOOR, 2), {std::string("###"), std::string("###"), '#', Block::PLANKS});
    addShapedRecipe(ItemStack(Item::IRON_DOOR, 1), {std::string("##"), std::string("##"), std::string("##"), '#', Item::IRON_INGOT});
    addShapedRecipe(ItemStack(Item::SIGN, 1),
        {std::string("###"), std::string("###"), std::string(" X "), '#', Block::PLANKS, 'X', Item::STICK});
    addShapedRecipe(ItemStack(Item::CAKE, 1),
        {std::string("AAA"), std::string("BEB"), std::string("CCC"), 'A', Item::MILK_BUCKET, 'B', Item::SUGAR, 'C', Item::WHEAT, 'E', Item::EGG});
    addShapedRecipe(ItemStack(Item::SUGAR, 1), {std::string("#"), '#', Item::SUGAR_CANE});
    addShapedRecipe(ItemStack(Block::PLANKS, 4), {std::string("#"), '#', Block::LOG});
    addShapedRecipe(ItemStack(Item::STICK, 4), {std::string("#"), std::string("#"), '#', Block::PLANKS});
    addShapedRecipe(ItemStack(Block::TORCH, 4), {std::string("X"), std::string("#"), 'X', Item::COAL, '#', Item::STICK});
    addShapedRecipe(ItemStack(Block::TORCH, 4),
        {std::string("X"), std::string("#"), 'X', ItemStack(Item::COAL, 1, 1), '#', Item::STICK});
    addShapedRecipe(ItemStack(Item::BOWL, 4), {std::string("# #"), std::string(" # "), '#', Block::PLANKS});
    addShapedRecipe(ItemStack(Block::RAIL, 16),
        {std::string("X X"), std::string("X#X"), std::string("X X"), 'X', Item::IRON_INGOT, '#', Item::STICK});
    addShapedRecipe(ItemStack(Block::POWERED_RAIL, 6),
        {std::string("X X"), std::string("X#X"), std::string("XRX"), 'X', Item::GOLD_INGOT, 'R', Item::REDSTONE, '#', Item::STICK});
    addShapedRecipe(ItemStack(Block::DETECTOR_RAIL, 6),
        {std::string("X X"), std::string("X#X"), std::string("XRX"), 'X', Item::IRON_INGOT, 'R', Item::REDSTONE, '#', Block::STONE_PRESSURE_PLATE});
    addShapedRecipe(ItemStack(Item::MINECART, 1), {std::string("# #"), std::string("###"), '#', Item::IRON_INGOT});
    addShapedRecipe(ItemStack(Block::JACK_O_LANTERN, 1), {std::string("A"), std::string("B"), 'A', Block::PUMPKIN, 'B', Block::TORCH});
    addShapedRecipe(ItemStack(Item::CHEST_MINECART, 1), {std::string("A"), std::string("B"), 'A', Block::CHEST, 'B', Item::MINECART});
    addShapedRecipe(ItemStack(Item::FURNACE_MINECART, 1), {std::string("A"), std::string("B"), 'A', Block::FURNACE, 'B', Item::MINECART});
    addShapedRecipe(ItemStack(Item::BOAT, 1), {std::string("# #"), std::string("###"), '#', Block::PLANKS});
    addShapedRecipe(ItemStack(Item::BUCKET, 1), {std::string("# #"), std::string(" # "), '#', Item::IRON_INGOT});
    addShapedRecipe(ItemStack(Item::FLINT_AND_STEEL, 1), {std::string("A "), std::string(" B"), 'A', Item::IRON_INGOT, 'B', Item::FLINT});
    addShapedRecipe(ItemStack(Item::BREAD, 1), {std::string("###"), '#', Item::WHEAT});
    addShapedRecipe(ItemStack(Block::WOODEN_STAIRS, 4),
        {std::string("#  "), std::string("## "), std::string("###"), '#', Block::PLANKS});
    addShapedRecipe(ItemStack(Item::FISHING_ROD, 1),
        {std::string("  #"), std::string(" #X"), std::string("# X"), '#', Item::STICK, 'X', Item::STRING});
    addShapedRecipe(ItemStack(Block::COBBLESTONE_STAIRS, 4),
        {std::string("#  "), std::string("## "), std::string("###"), '#', Block::COBBLESTONE});
    addShapedRecipe(ItemStack(Item::PAINTING, 1),
        {std::string("###"), std::string("#X#"), std::string("###"), '#', Item::STICK, 'X', Block::WOOL});
    addShapedRecipe(ItemStack(Item::GOLDEN_APPLE, 1),
        {std::string("###"), std::string("#X#"), std::string("###"), '#', Block::GOLD_BLOCK, 'X', Item::APPLE});
    addShapedRecipe(ItemStack(Block::LEVER, 1), {std::string("X"), std::string("#"), '#', Block::COBBLESTONE, 'X', Item::STICK});
    addShapedRecipe(ItemStack(Block::LIT_REDSTONE_TORCH, 1), {std::string("X"), std::string("#"), '#', Item::STICK, 'X', Item::REDSTONE});
    addShapedRecipe(ItemStack(Item::REPEATER, 1),
        {std::string("#X#"), std::string("III"), '#', Block::LIT_REDSTONE_TORCH, 'X', Item::REDSTONE, 'I', Block::STONE});
    addShapedRecipe(ItemStack(Item::CLOCK, 1),
        {std::string(" # "), std::string("#X#"), std::string(" # "), '#', Item::GOLD_INGOT, 'X', Item::REDSTONE});
    addShapedRecipe(ItemStack(Item::COMPASS, 1),
        {std::string(" # "), std::string("#X#"), std::string(" # "), '#', Item::IRON_INGOT, 'X', Item::REDSTONE});
    addShapedRecipe(ItemStack(Item::MAP, 1),
        {std::string("###"), std::string("#X#"), std::string("###"), '#', Item::PAPER, 'X', Item::COMPASS});
    addShapedRecipe(ItemStack(Block::BUTTON, 1), {std::string("#"), std::string("#"), '#', Block::STONE});
    addShapedRecipe(ItemStack(Block::STONE_PRESSURE_PLATE, 1), {std::string("##"), '#', Block::STONE});
    addShapedRecipe(ItemStack(Block::WOODEN_PRESSURE_PLATE, 1), {std::string("##"), '#', Block::PLANKS});
    addShapedRecipe(ItemStack(Block::DISPENSER, 1),
        {std::string("###"), std::string("#X#"), std::string("#R#"), '#', Block::COBBLESTONE, 'X', Item::BOW, 'R', Item::REDSTONE});
    addShapedRecipe(ItemStack(Block::PISTON, 1),
        {std::string("TTT"), std::string("#X#"), std::string("#R#"), '#', Block::COBBLESTONE, 'X', Item::IRON_INGOT, 'R', Item::REDSTONE, 'T', Block::PLANKS});
    addShapedRecipe(ItemStack(Block::STICKY_PISTON, 1), {std::string("S"), std::string("P"), 'S', Item::SLIMEBALL, 'P', Block::PISTON});
    addShapedRecipe(ItemStack(Item::BED, 1), {std::string("###"), std::string("XXX"), '#', Block::WOOL, 'X', Block::PLANKS});

    sortRecipes(recipes);
    std::cout << recipes.size() << " recipes" << std::endl;
}

void CraftingRecipeManager::addShapedRecipe(ItemStack output, std::vector<RecipeArg> input)
{
    std::string pattern;
    int width = 0;
    int height = 0;
    std::size_t index = 0;

    if (index < input.size() && input[index].kind() == RecipeArg::Kind::Rows) {
        for (const std::string& row : input[index].rows()) {
            ++height;
            width = static_cast<int>(row.length());
            pattern += row;
        }
        ++index;
    } else {
        while (index < input.size() && input[index].kind() == RecipeArg::Kind::Row) {
            const std::string& row = input[index].row();
            ++height;
            width = static_cast<int>(row.length());
            pattern += row;
            ++index;
        }
    }

    std::unordered_map<char, ItemStack> ingredients;
    while (index + 1 < input.size() && input[index].kind() == RecipeArg::Kind::Key) {
        const char key = input[index].key();
        const RecipeArg& value = input[index + 1];
        ItemStack stack;
        if (value.kind() == RecipeArg::Kind::Item && value.item() != nullptr) {
            stack = ItemStack(value.item(), 1, 0);
        } else if (value.kind() == RecipeArg::Kind::Block && value.block() != nullptr) {
            stack = ItemStack(value.block(), 1, -1);
        } else if (value.kind() == RecipeArg::Kind::Stack) {
            stack = value.stack().copy();
        }
        ingredients[key] = stack;
        index += 2;
    }

    std::vector<ItemStack> grid(static_cast<std::size_t>(width * height));
    for (int i = 0; i < width * height; ++i) {
        const char key = pattern[static_cast<std::size_t>(i)];
        const auto it = ingredients.find(key);
        grid[static_cast<std::size_t>(i)] = it != ingredients.end() ? it->second.copy() : ItemStack{};
    }
    recipes.push_back(std::make_unique<ShapedRecipe>(width, height, std::move(grid), output));
}

void CraftingRecipeManager::addShapelessRecipe(ItemStack output, std::vector<RecipeArg> input)
{
    std::vector<ItemStack> ingredients;
    ingredients.reserve(input.size());
    for (const RecipeArg& arg : input) {
        if (arg.kind() == RecipeArg::Kind::Stack) {
            ingredients.push_back(arg.stack().copy());
        } else if (arg.kind() == RecipeArg::Kind::Item && arg.item() != nullptr) {
            ingredients.push_back(ItemStack(arg.item()));
        } else if (arg.kind() == RecipeArg::Kind::Block && arg.block() != nullptr) {
            ingredients.push_back(ItemStack(arg.block()));
        } else {
            throw std::runtime_error("Invalid shapeless recipy!");
        }
    }
    recipes.push_back(std::make_unique<ShapelessRecipe>(output, std::move(ingredients)));
}

ItemStack CraftingRecipeManager::craft(const CraftingInventory& craftingInventory) const
{
    for (const std::unique_ptr<CraftingRecipe>& recipe : recipes) {
        if (recipe != nullptr && recipe->matches(craftingInventory)) {
            return recipe->craft(craftingInventory);
        }
    }
    return {};
}

} // namespace net::minecraft::recipe
