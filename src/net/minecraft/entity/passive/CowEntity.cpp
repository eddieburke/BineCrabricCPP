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
    if (itemStack != nullptr && Item::BUCKET != nullptr && itemStack->itemId == Item::BUCKET->id) {
        if (Item::MILK_BUCKET != nullptr) {
            player->inventory.setStack(player->inventory.selectedSlot, ItemStack(Item::MILK_BUCKET->id, 1, 0));
        }
        return true;
    }
    return false;
}

int CowEntity::getDroppedItemId() const
{
    return Item::LEATHER != nullptr ? Item::LEATHER->id : 334;
}

} // namespace net::minecraft::entity::passive
