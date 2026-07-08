#include "net/minecraft/block/BookshelfBlock.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"

namespace {
net::minecraft::BlockSoundGroup kWoodSound("wood", 1.0f, 1.0f);
}

namespace net::minecraft::block {
void BookshelfBlock::registerClass() {
    Block::BOOKSHELF = (new BookshelfBlock(kBlockId, 35))
                           ->setHardness(1.5f)
                           ->setSoundGroup(&kWoodSound)
                           ->setTranslationKey("bookshelf");
}

void BookshelfBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
    recipeManager.addShapedRecipe(
        ItemStack(Block::BOOKSHELF),
        {std::string("###"), std::string("XXX"), std::string("###"), '#', Block::PLANKS, 'X', Item::byRawId(84)});
}
MC_REGISTER_BLOCK(BookshelfBlock)
}  // namespace net::minecraft::block
