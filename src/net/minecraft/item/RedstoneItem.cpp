#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/RedstoneItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemPlacement.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

RedstoneItem::RedstoneItem(int rawId) : Item(rawId) {}

bool RedstoneItem::useOnBlock(ItemStack* stack, PlayerEntity* /*user*/, World* world, int x, int y, int z, int side)
{
    if (world == nullptr || stack == nullptr || Block::REDSTONE_WIRE == nullptr) {
        return false;
    }
    if (Block::SNOW == nullptr || world->getBlockId(x, y, z) != Block::SNOW->id) {
        detail::offsetPlacementPos(world, x, y, z, side);
        if (!world->isAir(x, y, z)) {
            return false;
        }
    }
    if (Block::REDSTONE_WIRE->canPlaceAt(world, x, y, z)) {
        --stack->count;
        world->setBlock(x, y, z, Block::REDSTONE_WIRE->id);
    }
    return true;
}

void RedstoneItem::registerClass()
{
    static RedstoneItem REDSTONE(75);
    REDSTONE.setTexturePosition(8, 3)->setTranslationKey("redstone");
}




namespace {static ::net::minecraft::registry::RegisterItem<RedstoneItem> autoReg; } // namespace

} // namespace net::minecraft::item
