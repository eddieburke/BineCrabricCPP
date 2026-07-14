#include "net/minecraft/block/PumpkinBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
namespace {
net::minecraft::BlockSoundGroup kWoodSound("wood", 1.0f, 1.0f);
}
namespace net::minecraft::block {
PumpkinBlock::PumpkinBlock(int blockId, int textureId, bool litIn)
    : Block(blockId, textureId, material::Material::PUMPKIN) {
  lit = litIn;
  setTickRandomly(true);
}
int PumpkinBlock::getTexture(int side, int meta) const {
  if(side == 1 || side == 0) {
    return textureId;
  }
  int faceTexture = textureId + 1 + 16;
  if(lit) {
    ++faceTexture;
  }
  if(meta == 2 && side == 2) {
    return faceTexture;
  }
  if(meta == 3 && side == 5) {
    return faceTexture;
  }
  if(meta == 0 && side == 3) {
    return faceTexture;
  }
  if(meta == 1 && side == 4) {
    return faceTexture;
  }
  return textureId + 16;
}
int PumpkinBlock::getTexture(int side) const {
  if(side == 1 || side == 0) {
    return textureId;
  }
  if(side == 3) {
    return textureId + 1 + 16;
  }
  return textureId + 16;
}
bool PumpkinBlock::canPlaceAt(World* world, int x, int y, int z) const {
  if(world == nullptr) {
    return false;
  }
  const int existingId = world->getBlockId(x, y, z);
  if(existingId != 0) {
    Block* existing = Block::BLOCKS[static_cast<std::size_t>(existingId)];
    if(existing == nullptr || !existing->material.isReplaceable()) {
      return false;
    }
  }
  return world->shouldSuffocate(x, y - 1, z);
}
void PumpkinBlock::onPlaced(World* world, int x, int y, int z, net::minecraft::entity::player::PlayerEntity* placer) {
  if(world == nullptr || placer == nullptr) {
    return;
  }
  const int facing = MathHelper::floor(static_cast<double>(placer->yaw * 4.0f / 360.0f) + 2.5) & 3;
  world->setBlockMeta(x, y, z, facing);
}
void PumpkinBlock::registerClass() {
  Block::PUMPKIN = (new PumpkinBlock(86, 102, false))
                       ->setHardness(1.0f)
                       ->setSoundGroup(&kWoodSound)
                       ->setTranslationKey("pumpkin")
                       ->ignoreMetaUpdates();
  Block::JACK_O_LANTERN = (new PumpkinBlock(kBlockId, 102, true))
                              ->setHardness(1.0f)
                              ->setSoundGroup(&kWoodSound)
                              ->setLuminance(1.0f)
                              ->setTranslationKey("litpumpkin")
                              ->ignoreMetaUpdates();
}
void PumpkinBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(ItemStack(Block::JACK_O_LANTERN),
                                {std::string("A"), std::string("B"), 'A', Block::PUMPKIN, 'B', Block::TORCH});
}
MC_REGISTER_BLOCK(PumpkinBlock)
} // namespace net::minecraft::block
