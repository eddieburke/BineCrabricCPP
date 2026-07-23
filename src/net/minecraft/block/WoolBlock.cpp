#include "net/minecraft/block/WoolBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/WoolBlockItem.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace {
net::minecraft::BlockSoundGroup kClothSound("cloth", 1.0f, 1.0f);
}
namespace net::minecraft::block {
void WoolBlock::registerClass() {
 Block::WOOL = (new WoolBlock())
                   ->setHardness(0.8f)
                   ->setSoundGroup(&kClothSound)
                   ->setTranslationKey("cloth")
                   ->ignoreMetaUpdates();
}
void WoolBlock::registerBlockItems() {
 (new item::WoolBlockItem(35 - 256))->setTranslationKey("cloth");
}
void WoolBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 recipeManager.addShapedRecipe(ItemStack(Block::WOOL),
                               {std::string("##"), std::string("##"), '#', Item::byRawId(31)});
}
MC_REGISTER_BLOCK(WoolBlock)
} // namespace net::minecraft::block
