#include "net/minecraft/block/SnowBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/SnowballItem.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/light/LightType.hpp"
namespace {
net::minecraft::BlockSoundGroup kClothSound("cloth", 1.0f, 1.0f);
}
namespace net::minecraft::block {
SnowBlock::SnowBlock(int id, int textureId) : Block(id, textureId, material::Material::SNOW_BLOCK) {
 setTickRandomly(true);
}
int SnowBlock::getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const {
 return Item::byRawId(76) != nullptr ? Item::byRawId(76)->id : 332;
}
void SnowBlock::onTick(World* world, int x, int y, int z, JavaRandom& /*random*/) {
 if(world == nullptr) {
  return;
 }
 if(world->getBrightness(LightType::Block, x, y, z) > 11) {
  dropStacks(world, x, y, z, world->getBlockMeta(x, y, z));
  world->setBlock(x, y, z, 0);
 }
}
void SnowBlock::registerClass() {
 Block::SNOW_BLOCK =
     (new SnowBlock(kBlockId, 66))->setHardness(0.2f)->setSoundGroup(&kClothSound)->setTranslationKey("snow");
}
void SnowBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
 recipeManager.addShapedRecipe(ItemStack(Block::SNOW_BLOCK),
                               {std::string("##"), std::string("##"), '#', Item::byRawId(76)});
}
MC_REGISTER_BLOCK(SnowBlock)
} // namespace net::minecraft::block
