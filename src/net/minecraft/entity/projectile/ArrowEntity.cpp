#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/EntityRegistrar.hpp"
#include "net/minecraft/world/World.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

#include "net/minecraft/entity/projectile/ArrowEntity.hpp"

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::entity::projectile {

ArrowEntity::ArrowEntity(World* world, LivingEntity* ownerIn) : Entity(world)
{
    owner = ownerIn;
    pickupAllowed = dynamic_cast<player::PlayerEntity*>(ownerIn) != nullptr;
    setBoundingBoxSpacing(0.5f, 0.5f);
    if (owner != nullptr) {
        setPositionAndAnglesKeepPrevAngles(
            owner->x,
            owner->y + static_cast<double>(owner->getEyeHeight()),
            owner->z,
            owner->yaw,
            owner->pitch);
        x -= static_cast<double>(MathHelper::cos(yaw / 180.0f * kPiF) * 0.16f);
        y -= 0.1;
        z -= static_cast<double>(MathHelper::sin(yaw / 180.0f * kPiF) * 0.16f);
        setPosition(x, y, z);
        standingEyeHeight = 0.0f;
        velocityX = -MathHelper::sin(yaw / 180.0f * kPiF) * MathHelper::cos(pitch / 180.0f * kPiF);
        velocityZ = MathHelper::cos(yaw / 180.0f * kPiF) * MathHelper::cos(pitch / 180.0f * kPiF);
        velocityY = -MathHelper::sin(pitch / 180.0f * kPiF);
        setProjectileVelocity(*this, velocityX, velocityY, velocityZ, 1.5f, 1.0f);
    }
}


void ArrowEntity::tick()
{
    Entity::tick();
    tickArrowProjectile(
        *this,
        owner,
        inAirTime,
        blockX,
        blockY,
        blockZ,
        blockId,
        blockMeta,
        inGround,
        life,
        shake,
        pickupAllowed);
}

void ArrowEntity::onPlayerInteraction(player::PlayerEntity* player)
{
    if (world == nullptr || world->isRemote() || player == nullptr) {
        return;
    }
    if (inGround && pickupAllowed && shake <= 0) {
        const int arrowId = Item::ARROW != nullptr ? Item::ARROW->id : 262;
        ItemStack pickupStack(arrowId, 1, 0);
        if (player->inventory.addStack(pickupStack)) {
            world->playSound(
                this,
                "random.pop",
                0.2f,
                ((random.nextFloat() - random.nextFloat()) * 0.7f + 1.0f) * 2.0f);
            world->notifyEntityPickup(this, player);
            markDead();
        }
    }
}


void ArrowEntity::registerClass()
{
    ::net::minecraft::entity::detail::registerVanillaEntity<ArrowEntity>("Arrow", 10);
}

static ::net::minecraft::registry::RegisterEntity<ArrowEntity> autoReg(10);

} // namespace net::minecraft::entity::projectile
