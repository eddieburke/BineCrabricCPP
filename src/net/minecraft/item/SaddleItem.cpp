#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/SaddleItem.hpp"

#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/passive/PigEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::item {

SaddleItem::SaddleItem(int rawId) : Item(rawId)
{
    setMaxCount(1);
}

void SaddleItem::useOnEntity(ItemStack* stack, LivingEntity* entity)
{
    auto* pig = dynamic_cast<entity::passive::PigEntity*>(entity);
    if (stack != nullptr && pig != nullptr && !pig->isSaddled()) {
        pig->setSaddled(true);
        --stack->count;
    }
}

bool SaddleItem::postHit(ItemStack* stack, LivingEntity* target, LivingEntity* /*attacker*/)
{
    useOnEntity(stack, target);
    return true;
}

void SaddleItem::registerClass()
{
    static SaddleItem SADDLE(73);
    SADDLE.setTexturePosition(8, 6)->setTranslationKey("saddle");
}





MC_REGISTER_ITEM(SaddleItem)
} // namespace net::minecraft::item
