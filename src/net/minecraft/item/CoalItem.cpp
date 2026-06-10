#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/CoalItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"

namespace net::minecraft::item {
namespace {

void CoalItem::registerClass()
{
    static CoalItem COAL(7);
    COAL.setTexturePosition(7, 0)->setTranslationKey("coal");
    Item::COAL = &COAL;
}




static ::net::minecraft::registry::RegisterItem<CoalItem> autoReg(7);
} // namespace
} // namespace net::minecraft::item
