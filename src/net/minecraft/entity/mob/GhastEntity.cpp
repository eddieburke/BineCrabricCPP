#include "net/minecraft/entity/mob/GhastEntity.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/projectile/FireballEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <cmath>

namespace net::minecraft::entity::mob {

GhastEntity::GhastEntity(World* world) : FlyingEntity(world)
{
    initDataTracker();
    texture = "/mob/ghast.png";
    setBoundingBoxSpacing(4.0f, 4.0f);
    fireImmune = true;
}

void GhastEntity::tick()
{
    FlyingEntity::tick();
    texture = dataTracker.getByte(16) == 1 ? "/mob/ghast_fire.png" : "/mob/ghast.png";
}

void GhastEntity::tickLiving()
{
    if (world != nullptr && !world->isRemote() && world->difficulty == 0) {
        markDead();
    }
    tryDespawn();
    lastChargeTime = chargeTime;

    const double deltaX = targetX - x;
    const double deltaY = targetY - y;
    const double deltaZ = targetZ - z;
    double distance = MathHelper::sqrt(static_cast<float>(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ));
    if (distance < 1.0 || distance > 60.0) {
        targetX = x + static_cast<double>((random.nextFloat() * 2.0f - 1.0f) * 16.0f);
        targetY = y + static_cast<double>((random.nextFloat() * 2.0f - 1.0f) * 16.0f);
        targetZ = z + static_cast<double>((random.nextFloat() * 2.0f - 1.0f) * 16.0f);
    }
    if (--floatDuration <= 0) {
        floatDuration += random.nextInt(5) + 2;
        if (canReach(targetX, targetY, targetZ, distance)) {
            if (distance > 1.0e-4) {
                velocityX += deltaX / distance * 0.1;
                velocityY += deltaY / distance * 0.1;
                velocityZ += deltaZ / distance * 0.1;
            }
        } else {
            targetX = x;
            targetY = y;
            targetZ = z;
        }
    }

    if (ghastTarget != nullptr && ghastTarget->dead) {
        ghastTarget = nullptr;
    }
    if (ghastTarget == nullptr || --angerCooldown <= 0) {
        ghastTarget = world != nullptr ? world->getClosestPlayer(this, 100.0) : nullptr;
        if (ghastTarget != nullptr) {
            angerCooldown = 20;
        }
    }

    constexpr double attackRange = 64.0;
    if (ghastTarget != nullptr && ghastTarget->getSquaredDistance(*this) < attackRange * attackRange) {
        const double toX = ghastTarget->x - x;
        const double toY = ghastTarget->boundingBox.minY + static_cast<double>(ghastTarget->height) * 0.5
            - (y + static_cast<double>(height) * 0.5);
        const double toZ = ghastTarget->z - z;
        bodyYaw = yaw = -static_cast<float>(std::atan2(toX, toZ) * 180.0 / 3.141592653589793);
        if (canSee(ghastTarget)) {
            if (chargeTime == 10 && world != nullptr) {
                world->playSound(this, "mob.ghast.charge", getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
            }
            ++chargeTime;
            if (chargeTime == 20 && world != nullptr) {
                world->playSound(this, "mob.ghast.fireball", getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
                auto* fireball = new projectile::FireballEntity(world, this, toX, toY, toZ);
                const Vec3d look = getLookVector(1.0f);
                constexpr double spawnOffset = 4.0;
                fireball->x = x + look.x * spawnOffset;
                fireball->y = y + static_cast<double>(height) * 0.5 + 0.5;
                fireball->z = z + look.z * spawnOffset;
                fireball->setPosition(fireball->x, fireball->y, fireball->z);
                world->spawnEntity(fireball);
                chargeTime = -40;
            }
        } else if (chargeTime > 0) {
            --chargeTime;
        }
    } else {
        bodyYaw = yaw = -static_cast<float>(std::atan2(velocityX, velocityZ) * 180.0 / 3.141592653589793);
        if (chargeTime > 0) {
            --chargeTime;
        }
    }

    if (world != nullptr && !world->isRemote()) {
        const std::int8_t current = dataTracker.getByte(16);
        const std::int8_t desired = chargeTime > 10 ? static_cast<std::int8_t>(1) : static_cast<std::int8_t>(0);
        if (current != desired) {
            dataTracker.set(16, desired);
        }
    }
}

bool GhastEntity::canReach(double targetXIn, double targetYIn, double targetZIn, double steps) const
{
    if (world == nullptr || steps < 1.0) {
        return true;
    }
    const double stepX = (targetXIn - x) / steps;
    const double stepY = (targetYIn - y) / steps;
    const double stepZ = (targetZIn - z) / steps;
    Box probe = boundingBox;
    for (int step = 1; step < static_cast<int>(steps); ++step) {
        probe = probe.translate(stepX, stepY, stepZ);
        if (!world->getEntityCollisions(const_cast<GhastEntity*>(this), probe).empty()) {
            return false;
        }
    }
    return true;
}

bool GhastEntity::canSpawn() const
{
    return random.nextInt(20) == 0 && FlyingEntity::canSpawn() && world != nullptr && world->difficulty > 0;
}

int GhastEntity::getDroppedItemId() const
{
    return Item::GUNPOWDER != nullptr ? Item::GUNPOWDER->id : 289;
}

} // namespace net::minecraft::entity::mob
