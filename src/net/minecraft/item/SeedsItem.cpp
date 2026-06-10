#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/SeedsItem.hpp"

namespace net::minecraft::item {
namespace {

void SeedsItem::registerClass()
{
    static SeedsItem SEEDS(39, 59);
    SEEDS.setTexturePosition(9, 0)->setTranslationKey("seeds");
    Item::SEEDS = &SEEDS;
}




static ::net::minecraft::registry::RegisterItem<SeedsItem> autoReg(39);
} // namespace
} // namespace net::minecraft::item
