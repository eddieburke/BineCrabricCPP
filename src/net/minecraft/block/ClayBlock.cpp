#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/ClayBlock.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
namespace {
net::minecraft::BlockSoundGroup kGravelSound("gravel", 1.0f, 1.0f);
}
namespace net::minecraft::block {
void ClayBlock::registerClass() {
  Block::CLAY = (new ClayBlock(kBlockId, 72))
                    ->setHardness(0.6f)
                    ->setSoundGroup(&kGravelSound)
                    ->setTranslationKey("clay");
}
void ClayBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Block::CLAY),
                                {std::string("##"), std::string("##"), '#', Item::byRawId(81)});
}
MC_REGISTER_BLOCK(ClayBlock)
} // namespace net::minecraft::block
