#include "net/minecraft/entity/vehicle/BoatEntity.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <cmath>

namespace net::minecraft::entity::vehicle {

BoatEntity::BoatEntity(World* world) : Entity(world)
{
    blocksSameBlockSpawning = true;
    setBoundingBoxSpacing(1.5f, 0.6f);
    standingEyeHeight = height / 2.0f;
}

BoatEntity::BoatEntity(World* world, double xIn, double yIn, double zIn) : BoatEntity(world)
{
    setPosition(xIn, yIn + static_cast<double>(standingEyeHeight), zIn);
    velocityX = 0.0;
    velocityY = 0.0;
    velocityZ = 0.0;
    prevX = xIn;
    prevY = yIn;
    prevZ = zIn;
}

std::optional<Box> BoatEntity::getCollisionAgainstShape(Entity* other) const
{
    if (other == nullptr) {
        return std::nullopt;
    }
    return other->boundingBox;
}

bool BoatEntity::damage(Entity* damageSource, int amount)
{
    (void)damageSource;
    if (world == nullptr || world->isRemote() || dead) {
        return true;
    }
    damageWobbleSide = -damageWobbleSide;
    damageWobbleTicks = 10;
    damageWobbleStrength += static_cast<float>(amount * 10);
    scheduleVelocityUpdate();
    if (damageWobbleStrength > 40.0f) {
        if (passenger != nullptr) {
            passenger->setVehicle(this);
        }
        const int planksId = Block::PLANKS != nullptr ? Block::PLANKS->id : 5;
        const int stickId = Item::STICK != nullptr ? Item::STICK->id : 280;
        for (int i = 0; i < 3; ++i) {
            dropItem(planksId, 1, 0.0f);
        }
        for (int i = 0; i < 2; ++i) {
            dropItem(stickId, 1, 0.0f);
        }
        markDead();
    }
    return true;
}

void BoatEntity::tick()
{
    Entity::tick();
    if (world == nullptr) {
        return;
    }
    if (damageWobbleTicks > 0) {
        --damageWobbleTicks;
    }
    if (damageWobbleStrength > 0.0f) {
        --damageWobbleStrength;
    }
    prevX = x;
    prevY = y;
    prevZ = z;

    double waterFraction = 0.0;
    constexpr int samples = 5;
    for (int i = 0; i < samples; ++i) {
        const double minY = boundingBox.minY + (boundingBox.maxY - boundingBox.minY) * static_cast<double>(i) / static_cast<double>(samples) - 0.125;
        const double maxY = boundingBox.minY + (boundingBox.maxY - boundingBox.minY) * static_cast<double>(i + 1) / static_cast<double>(samples) - 0.125;
        const Box probe {boundingBox.minX, minY, boundingBox.minZ, boundingBox.maxX, maxY, boundingBox.maxZ};
        if (world->isFluidInBox(probe, block::material::Material::WATER)) {
            waterFraction += 1.0 / static_cast<double>(samples);
        }
    }

    if (world->isRemote()) {
        if (clientInterpolationSteps > 0) {
            double yawDelta = clientPitch - static_cast<double>(yaw);
            while (yawDelta < -180.0) {
                yawDelta += 360.0;
            }
            while (yawDelta >= 180.0) {
                yawDelta -= 360.0;
            }
            const double interpX = x + (clientX - x) / static_cast<double>(clientInterpolationSteps);
            const double interpY = y + (clientY - y) / static_cast<double>(clientInterpolationSteps);
            const double interpZ = z + (clientZ - z) / static_cast<double>(clientInterpolationSteps);
            yaw = static_cast<float>(static_cast<double>(yaw) + yawDelta / static_cast<double>(clientInterpolationSteps));
            pitch = static_cast<float>(static_cast<double>(pitch) + (clientYaw - static_cast<double>(pitch)) / static_cast<double>(clientInterpolationSteps));
            --clientInterpolationSteps;
            setPosition(interpX, interpY, interpZ);
            setRotation(yaw, pitch);
        } else {
            setPosition(x + velocityX, y + velocityY, z + velocityZ);
            if (onGround) {
                velocityX *= 0.5;
                velocityY *= 0.5;
                velocityZ *= 0.5;
            }
            velocityX *= 0.99;
            velocityY *= 0.95;
            velocityZ *= 0.99;
        }
        return;
    }

    if (waterFraction < 1.0) {
        const double buoyancy = waterFraction * 2.0 - 1.0;
        velocityY += 0.04 * buoyancy;
    } else {
        if (velocityY < 0.0) {
            velocityY /= 2.0;
        }
        velocityY += 0.007;
    }
    if (passenger != nullptr) {
        velocityX += passenger->velocityX * 0.2;
        velocityZ += passenger->velocityZ * 0.2;
    }
    constexpr double maxSpeed = 0.4;
    velocityX = std::clamp(velocityX, -maxSpeed, maxSpeed);
    velocityZ = std::clamp(velocityZ, -maxSpeed, maxSpeed);
    if (onGround) {
        velocityX *= 0.5;
        velocityY *= 0.5;
        velocityZ *= 0.5;
    }
    move(velocityX, velocityY, velocityZ);
    const double horizontalSpeed = std::sqrt(velocityX * velocityX + velocityZ * velocityZ);
    if (horizontalSpeed > 0.15) {
        const double cosYaw = std::cos(static_cast<double>(yaw) * 3.141592653589793 / 180.0);
        const double sinYaw = std::sin(static_cast<double>(yaw) * 3.141592653589793 / 180.0);
        double particleStep = 0.0;
        while (particleStep < 1.0 + horizontalSpeed * 60.0) {
            const double spread = random.nextFloat() * 2.0f - 1.0f;
            const double lateral = static_cast<double>(random.nextInt(2) * 2 - 1) * 0.7;
            double particleX = 0.0;
            double particleZ = 0.0;
            if (random.nextInt(2) == 0) {
                particleX = x - cosYaw * spread * 0.8 + sinYaw * lateral;
                particleZ = z - sinYaw * spread * 0.8 - cosYaw * lateral;
            } else {
                particleX = x + cosYaw + sinYaw * spread * 0.7;
                particleZ = z + sinYaw - cosYaw * spread * 0.7;
            }
            world->addParticle("splash", particleX, y - 0.125, particleZ, velocityX, velocityY, velocityZ);
            particleStep += 1.0;
        }
    }
    if (horizontalCollision && horizontalSpeed > 0.15) {
        const int planksId = Block::PLANKS != nullptr ? Block::PLANKS->id : 5;
        const int stickId = Item::STICK != nullptr ? Item::STICK->id : 280;
        markDead();
        for (int i = 0; i < 3; ++i) {
            dropItem(planksId, 1, 0.0f);
        }
        for (int i = 0; i < 2; ++i) {
            dropItem(stickId, 1, 0.0f);
        }
        return;
    }
    velocityX *= 0.99;
    velocityY *= 0.95;
    velocityZ *= 0.99;

    pitch = 0.0f;
    double targetYaw = static_cast<double>(yaw);
    const double deltaX = prevX - x;
    const double deltaZ = prevZ - z;
    if (deltaX * deltaX + deltaZ * deltaZ > 0.001) {
        targetYaw = std::atan2(deltaZ, deltaX) * 180.0 / 3.141592653589793;
    }
    double yawDelta = targetYaw - static_cast<double>(yaw);
    while (yawDelta >= 180.0) {
        yawDelta -= 360.0;
    }
    while (yawDelta < -180.0) {
        yawDelta += 360.0;
    }
    if (yawDelta > 20.0) {
        yawDelta = 20.0;
    }
    if (yawDelta < -20.0) {
        yawDelta = -20.0;
    }
    yaw = static_cast<float>(static_cast<double>(yaw) + yawDelta);
    setRotation(yaw, pitch);

    const Box collisionBox = boundingBox.expand(0.2f, 0.0, 0.2f);
    for (Entity* entity : world->getEntities(this, collisionBox)) {
        if (entity == passenger || entity == nullptr || !entity->isPushable()) {
            continue;
        }
        if (dynamic_cast<BoatEntity*>(entity) != nullptr) {
            entity->onCollision(this);
        }
    }
    const int snowId = Block::SNOW != nullptr ? Block::SNOW->id : 78;
    for (int i = 0; i < 4; ++i) {
        const int snowX = MathHelper::floor(x + (static_cast<double>(i % 2) - 0.5) * 0.8);
        const int snowY = MathHelper::floor(y);
        const int snowZ = MathHelper::floor(z + (static_cast<double>(i / 2) - 0.5) * 0.8);
        if (world->getBlockId(snowX, snowY, snowZ) == snowId) {
            world->setBlock(snowX, snowY, snowZ, 0);
        }
    }
    if (passenger != nullptr && passenger->dead) {
        passenger = nullptr;
    }
}

void BoatEntity::setPositionAndAnglesAvoidEntities(
    double xIn,
    double yIn,
    double zIn,
    float yawIn,
    float pitchIn,
    int interpolationSteps)
{
    clientX = xIn;
    clientY = yIn;
    clientZ = zIn;
    clientPitch = static_cast<double>(yawIn);
    clientYaw = static_cast<double>(pitchIn);
    clientInterpolationSteps = interpolationSteps + 4;
    velocityX = clientVelocityX;
    velocityY = clientVelocityY;
    velocityZ = clientVelocityZ;
}

void BoatEntity::setVelocityClient(double xIn, double yIn, double zIn)
{
    clientVelocityX = xIn;
    clientVelocityY = yIn;
    clientVelocityZ = zIn;
    velocityX = xIn;
    velocityY = yIn;
    velocityZ = zIn;
}

void BoatEntity::updatePassengerPosition()
{
    if (passenger == nullptr) {
        return;
    }
    const double offsetX = std::cos(static_cast<double>(yaw) * 3.141592653589793 / 180.0) * 0.4;
    const double offsetZ = std::sin(static_cast<double>(yaw) * 3.141592653589793 / 180.0) * 0.4;
    passenger->setPosition(
        x + offsetX,
        y + getPassengerRidingHeight() + static_cast<double>(passenger->standingEyeHeight),
        z + offsetZ);
}

bool BoatEntity::interact(player::PlayerEntity* player)
{
    if (passenger != nullptr && dynamic_cast<player::PlayerEntity*>(passenger) != nullptr && passenger != player) {
        return true;
    }
    if (world != nullptr && !world->isRemote() && player != nullptr) {
        player->setVehicle(this);
    }
    return true;
}

void BoatEntity::onCollision(Entity* otherEntity)
{
    Entity::onCollision(otherEntity);
}

} // namespace net::minecraft::entity::vehicle
