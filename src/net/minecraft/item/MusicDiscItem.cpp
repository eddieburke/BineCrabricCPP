#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/MusicDiscItem.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::item {
MusicDiscItem::MusicDiscItem(int rawId, std::string soundIn) : Item(rawId), sound(std::move(soundIn)) {
  setMaxCount(1);
}
bool MusicDiscItem::useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z,
                               int /*side*/) {
  if(world == nullptr || stack == nullptr || Block::JUKEBOX == nullptr) {
    return false;
  }
  if(world->getBlockId(x, y, z) == Block::JUKEBOX->id && world->getBlockMeta(x, y, z) == 0) {
    if(world->isRemote()) {
      return true;
    }
    world->setBlockMetaWithoutNotifyingNeighbors(x, y, z, 1);
    world->playStreaming(sound, x, y, z);
    --stack->count;
    return true;
  }
  return false;
}
void MusicDiscItem::registerClass() {
  static MusicDiscItem RECORD_THIRTEEN(2000, "13");
  RECORD_THIRTEEN.setTexturePosition(0, 15)->setTranslationKey("record");
  static MusicDiscItem RECORD_CAT(2001, "cat");
  RECORD_CAT.setTexturePosition(1, 15)->setTranslationKey("record");
}
MC_REGISTER_ITEM(MusicDiscItem)
} // namespace net::minecraft::item
