#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"

namespace net::minecraft::item {
namespace {

void registerBasicItems()
{
    static Item DIAMOND(8);
    DIAMOND.setTexturePosition(7, 3)->setTranslationKey("emerald");
    Item::DIAMOND = &DIAMOND;

    static Item IRON_INGOT(9);
    IRON_INGOT.setTexturePosition(7, 1)->setTranslationKey("ingotIron");
    Item::IRON_INGOT = &IRON_INGOT;

    static Item GOLD_INGOT(10);
    GOLD_INGOT.setTexturePosition(7, 2)->setTranslationKey("ingotGold");
    Item::GOLD_INGOT = &GOLD_INGOT;

    static Item STICK(24);
    STICK.setTexturePosition(5, 3)->setHandheld()->setTranslationKey("stick");
    Item::STICK = &STICK;

    static Item BOWL(25);
    BOWL.setTexturePosition(7, 4)->setTranslationKey("bowl");
    Item::BOWL = &BOWL;

    static Item STRING(31);
    STRING.setTexturePosition(8, 0)->setTranslationKey("string");
    Item::STRING = &STRING;

    static Item FEATHER(32);
    FEATHER.setTexturePosition(8, 1)->setTranslationKey("feather");
    Item::FEATHER = &FEATHER;

    static Item GUNPOWDER(33);
    GUNPOWDER.setTexturePosition(8, 2)->setTranslationKey("sulphur");
    Item::GUNPOWDER = &GUNPOWDER;

    static Item WHEAT(40);
    WHEAT.setTexturePosition(9, 1)->setTranslationKey("wheat");
    Item::WHEAT = &WHEAT;

    static Item FLINT(62);
    FLINT.setTexturePosition(6, 0)->setTranslationKey("flint");
    Item::FLINT = &FLINT;

    static Item LEATHER(78);
    LEATHER.setTexturePosition(7, 6)->setTranslationKey("leather");
    Item::LEATHER = &LEATHER;

    static Item BRICK(80);
    BRICK.setTexturePosition(6, 1)->setTranslationKey("brick");
    Item::BRICK = &BRICK;

    static Item CLAY(81);
    CLAY.setTexturePosition(9, 3)->setTranslationKey("clay");
    Item::CLAY = &CLAY;

    static Item PAPER(83);
    PAPER.setTexturePosition(10, 3)->setTranslationKey("paper");
    Item::PAPER = &PAPER;

    static Item BOOK(84);
    BOOK.setTexturePosition(11, 3)->setTranslationKey("book");
    Item::BOOK = &BOOK;

    static Item SLIMEBALL(85);
    SLIMEBALL.setTexturePosition(14, 1)->setTranslationKey("slimeball");
    Item::SLIMEBALL = &SLIMEBALL;

    static Item COMPASS(89);
    COMPASS.setTexturePosition(6, 3)->setTranslationKey("compass");
    Item::COMPASS = &COMPASS;

    static Item CLOCK(91);
    CLOCK.setTexturePosition(6, 4)->setTranslationKey("clock");
    Item::CLOCK = &CLOCK;

    static Item GLOWSTONE_DUST(92);
    GLOWSTONE_DUST.setTexturePosition(9, 4)->setTranslationKey("yellowDust");
    Item::GLOWSTONE_DUST = &GLOWSTONE_DUST;

    static Item BONE(96);
    BONE.setTexturePosition(12, 1)->setHandheld()->setTranslationKey("bone");
    Item::BONE = &BONE;

    static Item SUGAR(97);
    SUGAR.setTexturePosition(13, 0)->setHandheld()->setTranslationKey("sugar");
    Item::SUGAR = &SUGAR;
}

MINECRAFT_REGISTER_ITEM(registerBasicItems, 8);

} // namespace
} // namespace net::minecraft::item
