#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/BlockRegistrar.hpp"
#include "net/minecraft/block/FenceBlock.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {
void FenceBlock::registerClass()
{
    Block::FENCE = (new FenceBlock(85, 4))->setHardness(2.0f)->setResistance(5.0f)->setSoundGroup(&vanillaWoodSound())->setTranslationKey("fence")->ignoreMetaUpdates();
}
void FenceBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::FENCE, 2),
        {std::string("###"), std::string("###"), '#', Item::byRawId(24)});
}





namespace {static ::net::minecraft::registry::RegisterBlock<FenceBlock> autoReg(85);} // namespace
} // namespace net::minecraft::block

