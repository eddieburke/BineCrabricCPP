#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/ClayBlock.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {
void ClayBlock::registerClass()
{
    Block::CLAY = (new ClayBlock(82, 72))->setHardness(0.6f)->setSoundGroup(&vanillaGravelSound())->setTranslationKey("clay");
}
void ClayBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::CLAY),
        {std::string("##"), std::string("##"), '#', Item::byRawId(81)});
}





namespace {static ::net::minecraft::registry::RegisterBlock<ClayBlock> autoReg(82);} // namespace
} // namespace net::minecraft::block

