#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/EntityRegistrar.hpp"
#include "net/minecraft/entity/passive/SquidEntity.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <cmath>

namespace net::minecraft::entity::passive {

SquidEntity::SquidEntity(World* world) : WaterCreatureEntity(world)
{
    texture = "/mob/squid.png";
    setBoundingBoxSpacing(0.95f, 0.95f);
    thrustTimerSpeed = 1.0f / (random.nextFloat() + 1.0f) * 0.2f;
}

bool SquidEntity::isSubmergedInWater() const
{
    if (world == nullptr) {
        return false;
    }
    return world->updateMovementInFluid(
        boundingBox.expand(0.0, -0.6, 0.0),
        block::material::Material::WATER,
        const_cast<SquidEntity*>(this));
}

void SquidEntity::tickMovement()
{
    WaterCreatureEntity::tickMovement();
    lastTiltAngle = tiltAngle;
    lastRollAngle = rollAngle;
    lastThrustTimer = thrustTimer;
    lastTentacleAngle = tentacleAngle;
    thrustTimer += thrustTimerSpeed;
    if (thrustTimer > kPiF * 2.0f) {
        thrustTimer -= kPiF * 2.0f;
        if (random.nextInt(10) == 0) {
            thrustTimerSpeed = 1.0f / (random.nextFloat() + 1.0f) * 0.2f;
        }
    }
    if (isSubmergedInWater()) {
        if (thrustTimer < kPiF) {
            const float phase = thrustTimer / kPiF;
            tentacleAngle = MathHelper::sin(phase * phase * kPiF) * kPiF * 0.25f;
            if (phase > 0.75f) {
                swimVelocityScale = 1.0f;
                turningSpeed = 1.0f;
            } else {
                turningSpeed *= 0.8f;
            }
        } else {
            tentacleAngle = 0.0f;
            swimVelocityScale *= 0.9f;
            turningSpeed *= 0.99f;
        }
        if (!interpolateOnly) {
            velocityX = static_cast<double>(swimX) * static_cast<double>(swimVelocityScale);
            velocityY = static_cast<double>(swimY) * static_cast<double>(swimVelocityScale);
            velocityZ = static_cast<double>(swimZ) * static_cast<double>(swimVelocityScale);
        }
        const float horizontal = MathHelper::sqrt(static_cast<float>(velocityX * velocityX + velocityZ * velocityZ));
        bodyYaw += (-static_cast<float>(std::atan2(velocityX, velocityZ) * 180.0 / 3.141592653589793) - bodyYaw) * 0.1f;
        yaw = bodyYaw;
        rollAngle += kPiF * turningSpeed * 1.5f;
        tiltAngle += (-static_cast<float>(std::atan2(horizontal, velocityY) * 180.0 / 3.141592653589793) - tiltAngle) * 0.1f;
    } else {
        tentacleAngle = std::abs(MathHelper::sin(thrustTimer)) * kPiF * 0.25f;
        if (!interpolateOnly) {
            velocityX = 0.0;
            velocityY -= 0.08;
            velocityY *= 0.98;
            velocityZ = 0.0;
        }
        tiltAngle += (-90.0f - tiltAngle) * 0.02f;
    }
}

void SquidEntity::travel(float /*sideways*/, float /*forward*/)
{
    move(velocityX, velocityY, velocityZ);
}

void SquidEntity::tickLiving()
{
    if (random.nextInt(50) == 0 || !submergedInWater || (swimX == 0.0f && swimY == 0.0f && swimZ == 0.0f)) {
        const float angle = random.nextFloat() * kPiF * 2.0f;
        swimX = MathHelper::cos(angle) * 0.2f;
        swimY = -0.1f + random.nextFloat() * 0.2f;
        swimZ = MathHelper::sin(angle) * 0.2f;
    }
    tryDespawn();
}

void SquidEntity::dropItems()
{
    const int count = random.nextInt(3) + 1;
    const int dyeId = Item::byRawId(95) != nullptr ? Item::byRawId(95)->id : 95;
    for (int i = 0; i < count; ++i) {
        dropItem(ItemStack(dyeId, 1, 0), 0.0f);
    }
}


void SquidEntity::registerClass()
{
    ::net::minecraft::entity::detail::registerVanillaEntity<SquidEntity>("Squid", 94);
}

static ::net::minecraft::registry::RegisterEntity<SquidEntity> autoReg(94);

} // namespace net::minecraft::entity::passive
