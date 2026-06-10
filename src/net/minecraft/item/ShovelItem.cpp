#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ShovelItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {
namespace {

void ShovelItem::registerClass()
{
    static ShovelItem IRON_SHOVEL(0, ToolMaterial::Iron);
    IRON_SHOVEL.setTexturePosition(2, 5)->setTranslationKey("shovelIron");
    Item::IRON_SHOVEL = &IRON_SHOVEL;

    static ShovelItem WOODEN_SHOVEL(13, ToolMaterial::Wood);
    WOODEN_SHOVEL.setTexturePosition(0, 5)->setTranslationKey("shovelWood");
    Item::WOODEN_SHOVEL = &WOODEN_SHOVEL;

    static ShovelItem STONE_SHOVEL(17, ToolMaterial::Stone);
    STONE_SHOVEL.setTexturePosition(1, 5)->setTranslationKey("shovelStone");
    Item::STONE_SHOVEL = &STONE_SHOVEL;

    static ShovelItem DIAMOND_SHOVEL(21, ToolMaterial::Diamond);
    DIAMOND_SHOVEL.setTexturePosition(3, 5)->setTranslationKey("shovelDiamond");
    Item::DIAMOND_SHOVEL = &DIAMOND_SHOVEL;

    static ShovelItem GOLDEN_SHOVEL(28, ToolMaterial::Gold);
    GOLDEN_SHOVEL.setTexturePosition(4, 5)->setTranslationKey("shovelGold");
    Item::GOLDEN_SHOVEL = &GOLDEN_SHOVEL;
}




static ::net::minecraft::registry::RegisterItem<ShovelItem> autoReg(0);
} // namespace
} // namespace net::minecraft::item
