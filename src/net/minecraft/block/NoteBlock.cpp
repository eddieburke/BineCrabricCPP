#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/block/NoteBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/NoteBlockBlockEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft::block {
NoteBlock::NoteBlock(int blockId) : BlockWithEntity(blockId, 74, material::Material::WOOD) {}
int NoteBlock::getTexture(int /*side*/) const {
  return textureId;
}
void NoteBlock::neighborUpdate(World* world, int x, int y, int z, int neighborId) {
  if(world == nullptr || neighborId <= 0 || neighborId >= Block::BLOCK_COUNT) {
    return;
  }
  Block* neighbor = Block::BLOCKS[static_cast<std::size_t>(neighborId)];
  if(neighbor == nullptr || !neighbor->canEmitRedstonePower()) {
    return;
  }
  const bool powered = world->canTransferPower(x, y, z);
  auto* noteBlock = dynamic_cast<entity::NoteBlockBlockEntity*>(world->getBlockEntity(x, y, z));
  if(noteBlock == nullptr || noteBlock->powered == powered) {
    return;
  }
  if(powered) {
    noteBlock->playNote(world, x, y, z);
  }
  noteBlock->powered = powered;
}
bool NoteBlock::onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* /*player*/) {
  if(world == nullptr || world->isRemote()) {
    return true;
  }
  auto* noteBlock = dynamic_cast<entity::NoteBlockBlockEntity*>(world->getBlockEntity(x, y, z));
  if(noteBlock == nullptr) {
    return true;
  }
  noteBlock->cycleNote();
  noteBlock->playNote(world, x, y, z);
  return true;
}
void NoteBlock::onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* /*player*/) {
  if(world == nullptr || world->isRemote()) {
    return;
  }
  auto* noteBlock = dynamic_cast<entity::NoteBlockBlockEntity*>(world->getBlockEntity(x, y, z));
  if(noteBlock != nullptr) {
    noteBlock->playNote(world, x, y, z);
  }
}
void NoteBlock::onBlockAction(World* world, int x, int y, int z, int data1, int data2) {
  if(world == nullptr) {
    return;
  }
  const float pitch = static_cast<float>(std::pow(2.0, static_cast<double>(data2 - 12) / 12.0));
  std::string instrument = "harp";
  if(data1 == 1) {
    instrument = "bd";
  } else if(data1 == 2) {
    instrument = "snare";
  } else if(data1 == 3) {
    instrument = "hat";
  } else if(data1 == 4) {
    instrument = "bassattack";
  }
  world->playSound(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5,
                   "note." + instrument, 3.0f, pitch);
  world->addParticle("note", static_cast<double>(x) + 0.5, static_cast<double>(y) + 1.2, static_cast<double>(z) + 0.5,
                     static_cast<double>(data2) / 24.0, 0.0, 0.0);
}
void NoteBlock::registerClass() {
  Block::NOTE_BLOCK =
      (new NoteBlock(kBlockId))->setHardness(0.8f)->setTranslationKey("musicBlock")->ignoreMetaUpdates();
}
void NoteBlock::registerRecipes(recipe::CraftingRecipeManager& recipeManager) {
  recipeManager.addShapedRecipe(
      ItemStack(Block::NOTE_BLOCK),
      {std::string("###"), std::string("#X#"), std::string("###"), '#', Block::PLANKS, 'X', Item::byRawId(75)});
}
MC_REGISTER_BLOCK(NoteBlock)
} // namespace net::minecraft::block
