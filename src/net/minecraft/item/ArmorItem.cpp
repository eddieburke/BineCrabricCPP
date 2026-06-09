#include "net/minecraft/item/ArmorItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"

namespace net::minecraft::item {
namespace {

void registerArmorItems()
{
    static ArmorItem LEATHER_HELMET(42, 0, 0, 0);
    LEATHER_HELMET.setTexturePosition(0, 0)->setTranslationKey("helmetCloth");
    Item::LEATHER_HELMET = &LEATHER_HELMET;
    static ArmorItem LEATHER_CHESTPLATE(43, 0, 0, 1);
    LEATHER_CHESTPLATE.setTexturePosition(0, 1)->setTranslationKey("chestplateCloth");
    Item::LEATHER_CHESTPLATE = &LEATHER_CHESTPLATE;
    static ArmorItem LEATHER_LEGGINGS(44, 0, 0, 2);
    LEATHER_LEGGINGS.setTexturePosition(0, 2)->setTranslationKey("leggingsCloth");
    Item::LEATHER_LEGGINGS = &LEATHER_LEGGINGS;
    static ArmorItem LEATHER_BOOTS(45, 0, 0, 3);
    LEATHER_BOOTS.setTexturePosition(0, 3)->setTranslationKey("bootsCloth");
    Item::LEATHER_BOOTS = &LEATHER_BOOTS;

    static ArmorItem CHAIN_HELMET(46, 1, 1, 0);
    CHAIN_HELMET.setTexturePosition(1, 0)->setTranslationKey("helmetChain");
    Item::CHAIN_HELMET = &CHAIN_HELMET;
    static ArmorItem CHAIN_CHESTPLATE(47, 1, 1, 1);
    CHAIN_CHESTPLATE.setTexturePosition(1, 1)->setTranslationKey("chestplateChain");
    Item::CHAIN_CHESTPLATE = &CHAIN_CHESTPLATE;
    static ArmorItem CHAIN_LEGGINGS(48, 1, 1, 2);
    CHAIN_LEGGINGS.setTexturePosition(1, 2)->setTranslationKey("leggingsChain");
    Item::CHAIN_LEGGINGS = &CHAIN_LEGGINGS;
    static ArmorItem CHAIN_BOOTS(49, 1, 1, 3);
    CHAIN_BOOTS.setTexturePosition(1, 3)->setTranslationKey("bootsChain");
    Item::CHAIN_BOOTS = &CHAIN_BOOTS;

    static ArmorItem IRON_HELMET(50, 2, 2, 0);
    IRON_HELMET.setTexturePosition(2, 0)->setTranslationKey("helmetIron");
    Item::IRON_HELMET = &IRON_HELMET;
    static ArmorItem IRON_CHESTPLATE(51, 2, 2, 1);
    IRON_CHESTPLATE.setTexturePosition(2, 1)->setTranslationKey("chestplateIron");
    Item::IRON_CHESTPLATE = &IRON_CHESTPLATE;
    static ArmorItem IRON_LEGGINGS(52, 2, 2, 2);
    IRON_LEGGINGS.setTexturePosition(2, 2)->setTranslationKey("leggingsIron");
    Item::IRON_LEGGINGS = &IRON_LEGGINGS;
    static ArmorItem IRON_BOOTS(53, 2, 2, 3);
    IRON_BOOTS.setTexturePosition(2, 3)->setTranslationKey("bootsIron");
    Item::IRON_BOOTS = &IRON_BOOTS;

    static ArmorItem DIAMOND_HELMET(54, 3, 3, 0);
    DIAMOND_HELMET.setTexturePosition(3, 0)->setTranslationKey("helmetDiamond");
    Item::DIAMOND_HELMET = &DIAMOND_HELMET;
    static ArmorItem DIAMOND_CHESTPLATE(55, 3, 3, 1);
    DIAMOND_CHESTPLATE.setTexturePosition(3, 1)->setTranslationKey("chestplateDiamond");
    Item::DIAMOND_CHESTPLATE = &DIAMOND_CHESTPLATE;
    static ArmorItem DIAMOND_LEGGINGS(56, 3, 3, 2);
    DIAMOND_LEGGINGS.setTexturePosition(3, 2)->setTranslationKey("leggingsDiamond");
    Item::DIAMOND_LEGGINGS = &DIAMOND_LEGGINGS;
    static ArmorItem DIAMOND_BOOTS(57, 3, 3, 3);
    DIAMOND_BOOTS.setTexturePosition(3, 3)->setTranslationKey("bootsDiamond");
    Item::DIAMOND_BOOTS = &DIAMOND_BOOTS;

    static ArmorItem GOLDEN_HELMET(58, 1, 4, 0);
    GOLDEN_HELMET.setTexturePosition(4, 0)->setTranslationKey("helmetGold");
    Item::GOLDEN_HELMET = &GOLDEN_HELMET;
    static ArmorItem GOLDEN_CHESTPLATE(59, 1, 4, 1);
    GOLDEN_CHESTPLATE.setTexturePosition(4, 1)->setTranslationKey("chestplateGold");
    Item::GOLDEN_CHESTPLATE = &GOLDEN_CHESTPLATE;
    static ArmorItem GOLDEN_LEGGINGS(60, 1, 4, 2);
    GOLDEN_LEGGINGS.setTexturePosition(4, 2)->setTranslationKey("leggingsGold");
    Item::GOLDEN_LEGGINGS = &GOLDEN_LEGGINGS;
    static ArmorItem GOLDEN_BOOTS(61, 1, 4, 3);
    GOLDEN_BOOTS.setTexturePosition(4, 3)->setTranslationKey("bootsGold");
    Item::GOLDEN_BOOTS = &GOLDEN_BOOTS;
}

MINECRAFT_REGISTER_ITEM(registerArmorItems, 42);

} // namespace
} // namespace net::minecraft::item
