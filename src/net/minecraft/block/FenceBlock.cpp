#include "net/minecraft/block/FenceBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace {
net::minecraft::BlockSoundGroup kWoodSound("wood", 1.0f, 1.0f);
}
namespace net::minecraft::block {
void FenceBlock::registerClass() {
  Block::FENCE = (new FenceBlock(kBlockId, 4))
                     ->setHardness(2.0f)
                     ->setResistance(5.0f)
                     ->setSoundGroup(&kWoodSound)
                     ->setTranslationKey("fence")
                     ->ignoreMetaUpdates();
}
void FenceBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Block::FENCE, 2),
                                {std::string("###"), std::string("###"), '#', Item::byRawId(24)});
}
MC_REGISTER_BLOCK(FenceBlock)
} // namespace net::minecraft::block
