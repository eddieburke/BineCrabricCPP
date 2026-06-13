#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/VanillaBlockSounds.hpp"
#include "net/minecraft/block/SandstoneBlock.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {
void SandstoneBlock::registerClass()
{
    Block::SANDSTONE = (new SandstoneBlock(kBlockId))->setSoundGroup(&vanillaStoneSound())->setHardness(0.8f)->setTranslationKey("sandStone");
}
void SandstoneBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::SANDSTONE),
        {std::string("##"), std::string("##"), '#', Block::SAND});
}





MC_REGISTER_BLOCK(SandstoneBlock)
} // namespace net::minecraft::block

