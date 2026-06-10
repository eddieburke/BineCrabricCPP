#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/EntityRegistrar.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity::passive {

SheepEntity::SheepEntity(World* world) : AnimalEntity(world)
{
    initDataTracker();
    texture = "/mob/sheep.png";
    setBoundingBoxSpacing(0.9f, 1.3f);
}

void SheepEntity::writeNbt(NbtCompound& nbt) const
{
    LivingEntity::writeNbt(nbt);
    nbt.putBoolean("Sheared", isSheared());
    nbt.putByte("Color", static_cast<std::int8_t>(getColor()));
}

void SheepEntity::readNbt(const NbtCompound& nbt)
{
    LivingEntity::readNbt(nbt);
    setSheared(nbt.getBoolean("Sheared"));
    setColor(static_cast<int>(nbt.getByte("Color")));
}

void SheepEntity::dropItems()
{
    if (!isSheared() && block::Block::WOOL != nullptr) {
        dropItem(ItemStack(block::Block::WOOL->id, 1, getColor()), 0.0f);
    }
}

bool SheepEntity::interact(player::PlayerEntity* player)
{
    if (player == nullptr) {
        return false;
    }
    ItemStack* itemStack = player->inventory.getSelectedItem();
    if (itemStack != nullptr && Item::SHEARS != nullptr && itemStack->itemId == Item::SHEARS->id && !isSheared()) {
        if (world != nullptr && !world->isRemote()) {
            setSheared(true);
            const int dropCount = 2 + random.nextInt(3);
            for (int i = 0; i < dropCount; ++i) {
                if (block::Block::WOOL == nullptr) {
                    break;
                }
                ItemEntity* itemEntity = dropItem(ItemStack(block::Block::WOOL->id, 1, getColor()), 1.0f);
                if (itemEntity != nullptr) {
                    itemEntity->velocityY += static_cast<double>(random.nextFloat() * 0.05f);
                    itemEntity->velocityX += static_cast<double>((random.nextFloat() - random.nextFloat()) * 0.1f);
                    itemEntity->velocityZ += static_cast<double>((random.nextFloat() - random.nextFloat()) * 0.1f);
                }
            }
        }
        itemStack->damageStack(1, player);
    }
    return false;
}


void SheepEntity::registerClass()
{
    ::net::minecraft::entity::detail::registerVanillaEntity<SheepEntity>("Sheep", 91);
}

static ::net::minecraft::registry::RegisterEntity<SheepEntity> autoReg(91);

} // namespace net::minecraft::entity::passive
