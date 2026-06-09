#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/AxeItem.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {
namespace {

void registerAxeItems()
{
    static AxeItem IRON_AXE(2, ToolMaterial::Iron);
    IRON_AXE.setTexturePosition(2, 7)->setTranslationKey("hatchetIron");
    Item::IRON_AXE = &IRON_AXE;

    static AxeItem WOODEN_AXE(15, ToolMaterial::Wood);
    WOODEN_AXE.setTexturePosition(0, 7)->setTranslationKey("hatchetWood");
    Item::WOODEN_AXE = &WOODEN_AXE;

    static AxeItem STONE_AXE(19, ToolMaterial::Stone);
    STONE_AXE.setTexturePosition(1, 7)->setTranslationKey("hatchetStone");
    Item::STONE_AXE = &STONE_AXE;

    static AxeItem DIAMOND_AXE(23, ToolMaterial::Diamond);
    DIAMOND_AXE.setTexturePosition(3, 7)->setTranslationKey("hatchetDiamond");
    Item::DIAMOND_AXE = &DIAMOND_AXE;

    static AxeItem GOLDEN_AXE(30, ToolMaterial::Gold);
    GOLDEN_AXE.setTexturePosition(4, 7)->setTranslationKey("hatchetGold");
    Item::GOLDEN_AXE = &GOLDEN_AXE;
}

MINECRAFT_REGISTER_ITEM(registerAxeItems, 2);

} // namespace
} // namespace net::minecraft::item
