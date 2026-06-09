#include "net/minecraft/item/CoalItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"

namespace net::minecraft::item {
namespace {

void registerCoalItem()
{
    static CoalItem COAL(7);
    COAL.setTexturePosition(7, 0)->setTranslationKey("coal");
    Item::COAL = &COAL;
}

MINECRAFT_REGISTER_ITEM(registerCoalItem, 7);

} // namespace
} // namespace net::minecraft::item
