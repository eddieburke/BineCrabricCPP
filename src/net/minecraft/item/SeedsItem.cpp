#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/SeedsItem.hpp"
namespace net::minecraft::item {
void SeedsItem::registerClass() {
  static SeedsItem instance(39, Block::WHEAT != nullptr ? Block::WHEAT->id : 59);
  instance.setTexturePosition(9, 0)->setTranslationKey("seeds");
  Item::registerInItemsArray(&instance);
}
MC_REGISTER_ITEM(SeedsItem)
} // namespace net::minecraft::item
