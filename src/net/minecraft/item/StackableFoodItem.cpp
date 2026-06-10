#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/StackableFoodItem.hpp"

namespace net::minecraft::item {
namespace {

void StackableFoodItem::registerClass()
{
    static StackableFoodItem COOKIE(101, 1, false, 8);
    COOKIE.setTexturePosition(12, 5)->setTranslationKey("cookie");
    Item::COOKIE = &COOKIE;
}




static ::net::minecraft::registry::RegisterItem<StackableFoodItem> autoReg(101);
} // namespace
} // namespace net::minecraft::item
