#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/VanillaBlockSounds.hpp"
#include "net/minecraft/block/WoolBlock.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/WoolBlockItem.hpp"

namespace net::minecraft::block {
void WoolBlock::registerClass()
{
    Block::WOOL = (new WoolBlock())->setHardness(0.8f)->setSoundGroup(&vanillaWoolSound())->setTranslationKey("cloth")->ignoreMetaUpdates();
}

void WoolBlock::registerBlockItems()
{
    (new item::WoolBlockItem(35 - 256))->setTranslationKey("cloth");
}
void WoolBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::WOOL),
        {std::string("##"), std::string("##"), '#', Item::byRawId(31)});
}





namespace {static ::net::minecraft::registry::RegisterBlock<WoolBlock> autoReg;} // namespace
} // namespace net::minecraft::block

