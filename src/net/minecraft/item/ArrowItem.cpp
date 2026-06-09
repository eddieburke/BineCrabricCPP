#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"

namespace net::minecraft::item {
namespace {

void registerArrowItem()
{
    static Item ARROW(6);
    ARROW.setTexturePosition(5, 2)->setTranslationKey("arrow");
    Item::ARROW = &ARROW;
}

MINECRAFT_REGISTER_ITEM(registerArrowItem, 6);

} // namespace
} // namespace net::minecraft::item
