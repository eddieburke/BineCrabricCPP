#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/SwordItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {
namespace {

void registerSwordItems()
{
    static SwordItem IRON_SWORD(11, ToolMaterial::Iron);
    IRON_SWORD.setTexturePosition(2, 4)->setTranslationKey("swordIron");
    Item::IRON_SWORD = &IRON_SWORD;

    static SwordItem WOODEN_SWORD(12, ToolMaterial::Wood);
    WOODEN_SWORD.setTexturePosition(0, 4)->setTranslationKey("swordWood");
    Item::WOODEN_SWORD = &WOODEN_SWORD;

    static SwordItem STONE_SWORD(16, ToolMaterial::Stone);
    STONE_SWORD.setTexturePosition(1, 4)->setTranslationKey("swordStone");
    Item::STONE_SWORD = &STONE_SWORD;

    static SwordItem DIAMOND_SWORD(20, ToolMaterial::Diamond);
    DIAMOND_SWORD.setTexturePosition(3, 4)->setTranslationKey("swordDiamond");
    Item::DIAMOND_SWORD = &DIAMOND_SWORD;

    static SwordItem GOLDEN_SWORD(27, ToolMaterial::Gold);
    GOLDEN_SWORD.setTexturePosition(4, 4)->setTranslationKey("swordGold");
    Item::GOLDEN_SWORD = &GOLDEN_SWORD;
}

MINECRAFT_REGISTER_ITEM(registerSwordItems, 11);

} // namespace
} // namespace net::minecraft::item
