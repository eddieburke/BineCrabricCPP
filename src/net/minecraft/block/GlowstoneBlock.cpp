#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/VanillaBlockSounds.hpp"
#include "net/minecraft/block/GlowstoneBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::block {
void GlowstoneBlock::registerClass()
{
    namespace mat = material;
    Block::GLOWSTONE = (new GlowstoneBlock(kBlockId, 105, mat::Material::STONE))->setHardness(0.3f)->setSoundGroup(&vanillaGlassSound())->setLuminance(1.0f)->setTranslationKey("lightgem");
}
void GlowstoneBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager)
{
    recipeManager.addShapedRecipe(ItemStack(Block::GLOWSTONE),
        {std::string("##"), std::string("##"), '#', Item::byRawId(92)});
}





namespace {static ::net::minecraft::registry::RegisterBlock<GlowstoneBlock> autoReg;} // namespace
} // namespace net::minecraft::block

