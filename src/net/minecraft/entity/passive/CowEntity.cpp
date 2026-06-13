#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/passive/CowEntity.hpp"



#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity::passive {

CowEntity::CowEntity(World* world) : AnimalEntity(world)
{
    texture = "/mob/cow.png";
    setBoundingBoxSpacing(0.9f, 1.3f);
}

bool CowEntity::interact(player::PlayerEntity* player)
{
    if (player == nullptr) {
        return false;
    }
    ItemStack* itemStack = player->inventory.getSelectedItem();
    if (itemStack != nullptr && Item::byRawId(69) != nullptr && itemStack->itemId == Item::byRawId(69)->id) {
        if (Item::byRawId(79) != nullptr) {
            player->inventory.setStack(player->inventory.selectedSlot, ItemStack(Item::byRawId(79)->id, 1, 0));
        }
        return true;
    }
    return false;
}

int CowEntity::getDroppedItemId() const
{
    return Item::byRawId(78) != nullptr ? Item::byRawId(78)->id : 334;
}



MC_REGISTER_ENTITY(CowEntity)
} // namespace net::minecraft::entity::passive
