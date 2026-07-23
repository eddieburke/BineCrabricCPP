#include "net/minecraft/block/StoneBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::block {
void StoneBlock::registerClass() {
 Block::STONE = (new StoneBlock(kBlockId, 1))->setHardness(1.5f)->setResistance(10.0f)->setTranslationKey("stone");
}
void StoneBlock::registerSmeltingRecipes() {
 if(Block::COBBLESTONE != nullptr && Block::STONE != nullptr) {
  recipe::SmeltingRecipeManager::instance().addRecipe(Block::COBBLESTONE->id, ItemStack(Block::STONE));
 }
}
MC_REGISTER_BLOCK(StoneBlock)
} // namespace net::minecraft::block
