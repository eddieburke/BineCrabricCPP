#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/HoeItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemPlacement.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

HoeItem::HoeItem(int rawId, ToolMaterial material) : Item(rawId)
{
    setMaxCount(1);
    setMaxDamage(toolMaterialDurability(material));
}

bool HoeItem::useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int side)
{
    if (stack == nullptr || world == nullptr || Block::FARMLAND == nullptr) {
        return false;
    }
    const int blockId = world->getBlockId(x, y, z);
    const int above = world->getBlockId(x, y + 1, z);
    if ((side != 0 && above == 0 && blockId == (Block::GRASS_BLOCK != nullptr ? Block::GRASS_BLOCK->id : -1))
        || blockId == (Block::DIRT != nullptr ? Block::DIRT->id : -1)) {
        detail::playPlaceSound(world, Block::FARMLAND, x, y, z);
        if (world->isRemote()) {
            return true;
        }
        world->setBlock(x, y, z, Block::FARMLAND->id);
        stack->applyDamage(1);
        return true;
    }
    return false;
}

namespace {

void HoeItem::registerClass()
{
    static HoeItem WOODEN_HOE(34, ToolMaterial::Wood);
    WOODEN_HOE.setTexturePosition(0, 8)->setTranslationKey("hoeWood");
    Item::WOODEN_HOE = &WOODEN_HOE;

    static HoeItem STONE_HOE(35, ToolMaterial::Stone);
    STONE_HOE.setTexturePosition(1, 8)->setTranslationKey("hoeStone");
    Item::STONE_HOE = &STONE_HOE;

    static HoeItem IRON_HOE(36, ToolMaterial::Iron);
    IRON_HOE.setTexturePosition(2, 8)->setTranslationKey("hoeIron");
    Item::IRON_HOE = &IRON_HOE;

    static HoeItem DIAMOND_HOE(37, ToolMaterial::Diamond);
    DIAMOND_HOE.setTexturePosition(3, 8)->setTranslationKey("hoeDiamond");
    Item::DIAMOND_HOE = &DIAMOND_HOE;

    static HoeItem GOLDEN_HOE(38, ToolMaterial::Gold);
    GOLDEN_HOE.setTexturePosition(4, 8)->setTranslationKey("hoeGold");
    Item::GOLDEN_HOE = &GOLDEN_HOE;
}




static ::net::minecraft::registry::RegisterItem<HoeItem> autoReg(34);
} // namespace

} // namespace net::minecraft::item
