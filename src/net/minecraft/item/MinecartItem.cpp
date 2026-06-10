#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/MinecartItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

MinecartItem::MinecartItem(int rawId, int type) : Item(rawId), type_(type)
{
    setMaxCount(1);
}

bool MinecartItem::useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int /*side*/)
{
    if (world == nullptr || stack == nullptr || Block::RAIL == nullptr || world->getBlockId(x, y, z) != Block::RAIL->id) {
        return false;
    }
    if (!world->isRemote()) {
        auto* minecart = new entity::vehicle::MinecartEntity(world);
        minecart->type = type_;
        minecart->setPosition(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5);
        world->spawnEntity(minecart);
    }
    --stack->count;
    return true;
}

namespace {

void MinecartItem::registerClass()
{
    static MinecartItem MINECART(72, 0);
    MINECART.setTexturePosition(7, 8)->setTranslationKey("minecart");
    Item::MINECART = &MINECART;

    static MinecartItem CHEST_MINECART(86, 1);
    CHEST_MINECART.setTexturePosition(7, 9)->setTranslationKey("minecartChest");
    Item::CHEST_MINECART = &CHEST_MINECART;

    static MinecartItem FURNACE_MINECART(87, 2);
    FURNACE_MINECART.setTexturePosition(7, 10)->setTranslationKey("minecartFurnace");
    Item::FURNACE_MINECART = &FURNACE_MINECART;
}




static ::net::minecraft::registry::RegisterItem<MinecartItem> autoReg(72);
} // namespace

} // namespace net::minecraft::item
