#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/SecondaryBlockItem.hpp"

namespace net::minecraft::item {
namespace {

void registerSecondaryBlockItems()
{
    static SecondaryBlockItem SUGAR_CANE(82, Block::SUGAR_CANE);
    SUGAR_CANE.setTexturePosition(11, 1)->setTranslationKey("reeds");
    Item::SUGAR_CANE = &SUGAR_CANE;

    static SecondaryBlockItem CAKE(98, Block::CAKE);
    CAKE.setMaxCount(1)->setTexturePosition(13, 1)->setTranslationKey("cake");
    Item::CAKE = &CAKE;

    static SecondaryBlockItem REPEATER(100, Block::REPEATER);
    REPEATER.setTexturePosition(6, 5)->setTranslationKey("diode");
    Item::REPEATER = &REPEATER;
}

MINECRAFT_REGISTER_ITEM(registerSecondaryBlockItems, 82);

} // namespace
} // namespace net::minecraft::item
