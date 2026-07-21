#include "net/minecraft/block/GlowstoneBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::block {
void GlowstoneBlock::registerClass() {
 namespace mat = material;
 Block::GLOWSTONE = (new GlowstoneBlock(kBlockId, 105, mat::Material::STONE))
                        ->setHardness(0.3f)
                        ->setSoundGroup(&Block::GLASS_BLOCK_SOUNDS)
                        ->setLuminance(1.0f)
                        ->setTranslationKey("lightgem");
}
void GlowstoneBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 recipeManager.addShapedRecipe(ItemStack(Block::GLOWSTONE),
                               {std::string("##"), std::string("##"), '#', Item::byRawId(92)});
}
MC_REGISTER_BLOCK(GlowstoneBlock)
} // namespace net::minecraft::block
