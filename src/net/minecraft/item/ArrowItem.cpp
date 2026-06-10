#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"

namespace net::minecraft::item {
namespace {

void ArrowItem::registerClass()
{
    static Item ARROW(6);
    ARROW.setTexturePosition(5, 2)->setTranslationKey("arrow");
    Item::ARROW = &ARROW;
}




static ::net::minecraft::registry::RegisterItem<ArrowItem> autoReg(6);
} // namespace
} // namespace net::minecraft::item
