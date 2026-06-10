#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/MushroomStewItem.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::item {

MushroomStewItem::MushroomStewItem(int rawId, int healthRestored)
    : FoodItem(rawId, healthRestored, false)
{
    setMaxCount(1);
}

ItemStack* MushroomStewItem::use(ItemStack* stack, World* world, PlayerEntity* user)
{
    FoodItem::use(stack, world, user);
    return new ItemStack(Item::BOWL);
}

namespace {

void MushroomStewItem::registerClass()
{
    static MushroomStewItem MUSHROOM_STEW(26, 10);
    MUSHROOM_STEW.setTexturePosition(8, 4)->setTranslationKey("mushroomStew");
    Item::MUSHROOM_STEW = &MUSHROOM_STEW;
}




static ::net::minecraft::registry::RegisterItem<MushroomStewItem> autoReg(26);
} // namespace

} // namespace net::minecraft::item
