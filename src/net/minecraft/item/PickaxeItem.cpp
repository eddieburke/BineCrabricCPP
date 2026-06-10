#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/PickaxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {
namespace {

void PickaxeItem::registerClass()
{
    static PickaxeItem IRON_PICKAXE(1, ToolMaterial::Iron);
    IRON_PICKAXE.setTexturePosition(2, 6)->setTranslationKey("pickaxeIron");
    Item::IRON_PICKAXE = &IRON_PICKAXE;

    static PickaxeItem WOODEN_PICKAXE(14, ToolMaterial::Wood);
    WOODEN_PICKAXE.setTexturePosition(0, 6)->setTranslationKey("pickaxeWood");
    Item::WOODEN_PICKAXE = &WOODEN_PICKAXE;

    static PickaxeItem STONE_PICKAXE(18, ToolMaterial::Stone);
    STONE_PICKAXE.setTexturePosition(1, 6)->setTranslationKey("pickaxeStone");
    Item::STONE_PICKAXE = &STONE_PICKAXE;

    static PickaxeItem DIAMOND_PICKAXE(22, ToolMaterial::Diamond);
    DIAMOND_PICKAXE.setTexturePosition(3, 6)->setTranslationKey("pickaxeDiamond");
    Item::DIAMOND_PICKAXE = &DIAMOND_PICKAXE;

    static PickaxeItem GOLDEN_PICKAXE(29, ToolMaterial::Gold);
    GOLDEN_PICKAXE.setTexturePosition(4, 6)->setTranslationKey("pickaxeGold");
    Item::GOLDEN_PICKAXE = &GOLDEN_PICKAXE;
}




static ::net::minecraft::registry::RegisterItem<PickaxeItem> autoReg(1);
} // namespace
} // namespace net::minecraft::item
