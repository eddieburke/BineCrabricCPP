#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/EntityRegistrar.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"

#include "net/minecraft/entity/EntityRegistry.hpp"

#include <memory>
#include <typeindex>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/movement/PlayerMovement.hpp"
#include "net/minecraft/sound/BlockSoundGroup.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace net::minecraft::entity {

LivingEntity::LivingEntity(World* world)
    : Entity(world)
{
    blocksSameBlockSpawning = true;
    field1011 = static_cast<float>(random.nextDouble() + 1.0) * 0.01f;
    setPosition(x, y, z);
    field1010 = static_cast<float>(random.nextDouble() * 12398.0);
    yaw = static_cast<float>(random.nextDouble() * 3.14159265 * 2.0);
    stepHeight = 0.5f;
    field1047 = static_cast<float>(random.nextDouble() * 0.9 + 0.1);
}

std::string LivingEntity::getTexture() const
{
    return texture;
}

bool LivingEntity::isCollidable() const
{
    return !dead;
}

bool LivingEntity::isPushable() const
{
    return !dead;
}

float LivingEntity::getEyeHeight() const
{
    return height * 0.85f;
}

int LivingEntity::getMinAmbientSoundDelay() const
{
    return 80;
}

void LivingEntity::makeSound()
{
    const std::string sound = getRandomSound();
    if (!sound.empty() && world != nullptr) {
        world->playSound(this, sound, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
    }
}

void LivingEntity::baseTick()
{
    lastSwingAnimationProgress = swingAnimationProgress;
    Entity::baseTick();
    if (random.nextInt(1000) < ambientSoundTimer++) {
        ambientSoundTimer = -getMinAmbientSoundDelay();
        makeSound();
    }
    if (isAlive() && isInsideWall()) {
        damage(nullptr, 1);
    }
    if (fireImmune || (world != nullptr && world->isRemote())) {
        fireTicks = 0;
    }
    if (isAlive() && world != nullptr
        && isInFluid(block::material::Material::WATER) && !canBreatheInWater()) {
        --air;
        if (air == -20) {
            air = 0;
            for (int i = 0; i < 8; ++i) {
                const float offsetX = random.nextFloat() - random.nextFloat();
                const float offsetY = random.nextFloat() - random.nextFloat();
                const float offsetZ = random.nextFloat() - random.nextFloat();
                world->addParticle(
                    "bubble",
                    x + static_cast<double>(offsetX),
                    y + static_cast<double>(offsetY),
                    z + static_cast<double>(offsetZ),
                    velocityX,
                    velocityY,
                    velocityZ);
            }
            damage(nullptr, 2);
        }
        fireTicks = 0;
    } else {
        air = maxAir;
    }
    if (attackCooldown > 0) {
        --attackCooldown;
    }
    if (hurtTime > 0) {
        --hurtTime;
    }
    if (hearts > 0) {
        --hearts;
    }
    if (health <= 0) {
        ++deathTime;
        if (deathTime > 20) {
            beforeRemove();
            if (world != nullptr) {
                for (int i = 0; i < 20; ++i) {
                    const double particleDx = random.nextGaussian() * 0.02;
                    const double particleDy = random.nextGaussian() * 0.02;
                    const double particleDz = random.nextGaussian() * 0.02;
                    world->addParticle(
                        "explode",
                        x + static_cast<double>(random.nextFloat() * width * 2.0f - width),
                        y + static_cast<double>(random.nextFloat() * height),
                        z + static_cast<double>(random.nextFloat() * width * 2.0f - width),
                        particleDx,
                        particleDy,
                        particleDz);
                }
            }
            markDead();
        }
    }
    field1017 = field1016;
    lastBodyYaw = bodyYaw;
    prevYaw = yaw;
    prevPitch = pitch;
    prevTilt = tilt;
}

void LivingEntity::tick()
{
    Entity::tick();
    tickMovement();

    const double deltaX = x - prevX;
    const double deltaZ = z - prevZ;
    const float horizontal = MathHelper::sqrt(static_cast<float>(deltaX * deltaX + deltaZ * deltaZ));
    float newBodyYaw = bodyYaw;
    float fieldValue = 0.0f;
    lastWalkProgress = walkProgress;
    float walkTarget = 0.0f;

    if (horizontal > 0.05f) {
        walkTarget = 1.0f;
        fieldValue = horizontal * 3.0f;
        newBodyYaw = static_cast<float>(std::atan2(deltaZ, deltaX) * 180.0 / kPiF) - 90.0f;
    }
    if (swingAnimationProgress > 0.0f) {
        newBodyYaw = yaw;
    }
    if (!onGround) {
        walkTarget = 0.0f;
    }

    walkProgress += (walkTarget - walkProgress) * 0.3f;

    float yawDelta = newBodyYaw - bodyYaw;
    while (yawDelta < -180.0f) {
        yawDelta += 360.0f;
    }
    while (yawDelta >= 180.0f) {
        yawDelta -= 360.0f;
    }
    bodyYaw += yawDelta * 0.3f;

    float headDelta = yaw - bodyYaw;
    while (headDelta < -180.0f) {
        headDelta += 360.0f;
    }
    while (headDelta >= 180.0f) {
        headDelta -= 360.0f;
    }
    const bool backwards = headDelta < -90.0f || headDelta >= 90.0f;
    headDelta = std::clamp(headDelta, -75.0f, 75.0f);
    bodyYaw = yaw - headDelta;
    if (headDelta * headDelta > 2500.0f) {
        bodyYaw += headDelta * 0.2f;
    }
    if (backwards) {
        fieldValue *= -1.0f;
    }

    while (yaw - prevYaw < -180.0f) {
        prevYaw -= 360.0f;
    }
    while (yaw - prevYaw >= 180.0f) {
        prevYaw += 360.0f;
    }
    while (bodyYaw - lastBodyYaw < -180.0f) {
        lastBodyYaw -= 360.0f;
    }
    while (bodyYaw - lastBodyYaw >= 180.0f) {
        lastBodyYaw += 360.0f;
    }
    while (pitch - prevPitch < -180.0f) {
        prevPitch -= 360.0f;
    }
    while (pitch - prevPitch >= 180.0f) {
        prevPitch += 360.0f;
    }
    bodyYaw += fieldValue;
}

void LivingEntity::tickRiding()
{
    Entity::tickRiding();
    lastWalkProgress = walkProgress;
    walkProgress = 0.0f;
}

void LivingEntity::heal(int amount)
{
    if (health <= 0) {
        return;
    }
    health += amount;
    if (health > 20) {
        health = 20;
    }
    hearts = maxHealth / 2;
}

bool LivingEntity::damage(Entity* damageSource, int amount)
{
    if (world != nullptr && world->isRemote()) {
        return false;
    }
    if (health <= 0) {
        return false;
    }
    despawnCounter = 0;
    walkAnimationSpeed = 1.5f;
    bool updateEffects = true;
    if (hearts > maxHealth / 2) {
        if (amount <= prevHealth) {
            return false;
        }
        applyDamage(amount - prevHealth);
        prevHealth = amount;
        updateEffects = false;
    } else {
        prevHealth = amount;
        lastHealth = health;
        hearts = maxHealth;
        applyDamage(amount);
        damagedTime = 10;
        hurtTime = 10;
    }
    damagedSwingDir = 0.0f;
    if (updateEffects) {
        if (world != nullptr) {
            world->broadcastEntityEvent(this, 2);
        }
        scheduleVelocityUpdate();
        if (damageSource != nullptr) {
            double dx = damageSource->x - x;
            double dz = damageSource->z - z;
            while (dx * dx + dz * dz < 1.0E-4) {
                dx = (random.nextDouble() - random.nextDouble()) * 0.01;
                dz = (random.nextDouble() - random.nextDouble()) * 0.01;
            }
            damagedSwingDir = static_cast<float>(std::atan2(dz, dx) * 180.0 / kPiF) - yaw;
            applyKnockback(damageSource, amount, dx, dz);
        } else {
            damagedSwingDir = static_cast<float>(random.nextInt(2) * 180);
        }
    }
    if (health <= 0) {
        if (updateEffects && world != nullptr) {
            const std::string deathSound = getDeathSound();
            if (!deathSound.empty()) {
                world->playSound(this, deathSound, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
            }
        }
        onKilledBy(damageSource);
    } else if (updateEffects && world != nullptr) {
        const std::string hurtSound = getHurtSound();
        if (!hurtSound.empty()) {
            world->playSound(this, hurtSound, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
        }
    }
    return true;
}

void LivingEntity::animateHurt()
{
    damagedTime = 10;
    hurtTime = 10;
    damagedSwingDir = 0.0f;
}

void LivingEntity::animateSpawn()
{
    if (world == nullptr) {
        return;
    }
    for (int i = 0; i < 20; ++i) {
        const double particleDx = random.nextGaussian() * 0.02;
        const double particleDy = random.nextGaussian() * 0.02;
        const double particleDz = random.nextGaussian() * 0.02;
        constexpr double spread = 10.0;
        world->addParticle(
            "explode",
            x + static_cast<double>(random.nextFloat() * width * 2.0f - width) - particleDx * spread,
            y + static_cast<double>(random.nextFloat() * height) - particleDy * spread,
            z + static_cast<double>(random.nextFloat() * width * 2.0f - width) - particleDz * spread,
            particleDx,
            particleDy,
            particleDz);
    }
}

void LivingEntity::applyDamage(int amount)
{
    health -= amount;
}

float LivingEntity::getSoundVolume() const
{
    return 1.0f;
}

std::string LivingEntity::getRandomSound()
{
    return {};
}

std::string LivingEntity::getHurtSound() const
{
    return "random.hurt";
}

std::string LivingEntity::getDeathSound() const
{
    return "random.hurt";
}

void LivingEntity::applyKnockback(Entity* attacker, int amount, double dx, double dz)
{
    (void)attacker;
    (void)amount;
    const float magnitude = MathHelper::sqrt(dx * dx + dz * dz);
    constexpr float strength = 0.4f;
    velocityX /= 2.0;
    velocityY /= 2.0;
    velocityZ /= 2.0;
    velocityX -= dx / static_cast<double>(magnitude) * static_cast<double>(strength);
    velocityY += 0.4;
    velocityZ -= dz / static_cast<double>(magnitude) * static_cast<double>(strength);
    if (velocityY > 0.4) {
        velocityY = 0.4;
    }
}

void LivingEntity::onKilledBy(Entity* adversary)
{
    if (scoreAmount >= 0 && adversary != nullptr) {
        adversary->updateKilledAchievement(this, scoreAmount);
    }
    if (adversary != nullptr) {
        adversary->onKilledOther(this);
    }
    killedByOtherEntity = true;
    if (world != nullptr && !world->isRemote()) {
        dropItems();
        world->broadcastEntityEvent(this, 3);
    }
}

bool LivingEntity::canSee(Entity* entity) const
{
    if (world == nullptr || entity == nullptr) {
        return false;
    }
    const Vec3d from {x, y + static_cast<double>(getEyeHeight()), z};
    const Vec3d to {entity->x, entity->y + static_cast<double>(entity->getEyeHeight()), entity->z};
    return !world->raycast(from, to).has_value();
}

void LivingEntity::dropItems()
{
    if (world != nullptr && world->isRemote()) {
        return;
    }
    const int itemId = getDroppedItemId();
    if (itemId > 0) {
        const int dropCount = random.nextInt(3);
        for (int i = 0; i < dropCount; ++i) {
            dropItem(itemId, 1);
        }
    }
}

int LivingEntity::getDroppedItemId() const
{
    return 0;
}

void LivingEntity::onLanding(float fallDistanceIn)
{
    Entity::onLanding(fallDistanceIn);
    const int damageAmount = static_cast<int>(std::ceil(fallDistanceIn - 3.0f));
    if (damageAmount > 0) {
        damage(nullptr, damageAmount);
        if (world != nullptr) {
            const int blockId = world->getBlockId(
                MathHelper::floor(x),
                MathHelper::floor(y - 0.2 - static_cast<double>(standingEyeHeight)),
                MathHelper::floor(z));
            if (blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
                && Block::BLOCKS[static_cast<std::size_t>(blockId)]->soundGroup != nullptr) {
                BlockSoundGroup* soundGroup = Block::BLOCKS[static_cast<std::size_t>(blockId)]->soundGroup;
                world->playSound(
                    this,
                    soundGroup->getSound(),
                    soundGroup->getVolume() * 0.5f,
                    soundGroup->getPitch() * 0.75f);
            }
        }
    }
}

void LivingEntity::travel(float sideways, float forward)
{
    movement::PlayerMovement::travel(*this, sideways, forward);
}

bool LivingEntity::isOnLadder() const
{
    if (world == nullptr) {
        return false;
    }
    const int blockX = MathHelper::floor(x);
    const int blockY = MathHelper::floor(boundingBox.minY);
    const int blockZ = MathHelper::floor(z);
    return Block::LADDER != nullptr && world->getBlockId(blockX, blockY, blockZ) == Block::LADDER->id;
}

void LivingEntity::writeNbt(NbtCompound& nbt) const
{
    Entity::writeNbt(nbt);
    nbt.putShort("Health", static_cast<std::int16_t>(health));
    nbt.putShort("HurtTime", static_cast<std::int16_t>(hurtTime));
    nbt.putShort("DeathTime", static_cast<std::int16_t>(deathTime));
    nbt.putShort("AttackTime", static_cast<std::int16_t>(attackCooldown));
}

void LivingEntity::readNbt(const NbtCompound& nbt)
{
    Entity::readNbt(nbt);
    health = nbt.getShort("Health");
    if (!nbt.contains("Health")) {
        health = 10;
    }
    hurtTime = nbt.getShort("HurtTime");
    deathTime = nbt.getShort("DeathTime");
    attackCooldown = nbt.getShort("AttackTime");
}

bool LivingEntity::isAlive() const
{
    return !dead && health > 0;
}

bool LivingEntity::canBreatheInWater() const
{
    return false;
}

void LivingEntity::tickMovement()
{
    if (bodyTrackingIncrements > 0) {
        const double newX = x + (lerpX - x) / static_cast<double>(bodyTrackingIncrements);
        const double newY = y + (lerpY - y) / static_cast<double>(bodyTrackingIncrements);
        const double newZ = z + (lerpZ - z) / static_cast<double>(bodyTrackingIncrements);
        double deltaYaw = lerpYaw - static_cast<double>(yaw);
        while (deltaYaw < -180.0) {
            deltaYaw += 360.0;
        }
        while (deltaYaw >= 180.0) {
            deltaYaw -= 360.0;
        }
        yaw = static_cast<float>(static_cast<double>(yaw) + deltaYaw / static_cast<double>(bodyTrackingIncrements));
        pitch = static_cast<float>(static_cast<double>(pitch) + (lerpPitch - static_cast<double>(pitch)) / static_cast<double>(bodyTrackingIncrements));
        --bodyTrackingIncrements;
        setPosition(newX, newY, newZ);
        setRotation(yaw, pitch);
        if (world != nullptr) {
            const std::vector<Box> interpolationCollisions =
                world->getEntityCollisions(this, boundingBox.contract(0.03125, 0.0, 0.03125));
            if (!interpolationCollisions.empty()) {
                double highestCollision = 0.0;
                for (const Box& box : interpolationCollisions) {
                    if (box.maxY > highestCollision) {
                        highestCollision = box.maxY;
                    }
                }
                setPosition(newX, newY + highestCollision - boundingBox.minY, newZ);
            }
        }
    }

    if (isImmobile()) {
        jumping = false;
        sidewaysSpeed = 0.0f;
        forwardSpeed = 0.0f;
        rotationSpeed = 0.0f;
    } else if (!interpolateOnly) {
        tickLiving();
    }

    movement::PlayerMovement::applyJumpInput(*this);

    sidewaysSpeed *= 0.98f;
    forwardSpeed *= 0.98f;
    rotationSpeed *= 0.9f;
    travel(sidewaysSpeed, forwardSpeed);

    if (world != nullptr) {
        const std::vector<Entity*> nearby = world->getEntities(this, boundingBox.expand(0.2));
        for (Entity* entity : nearby) {
            if (entity != nullptr && entity->isPushable()) {
                entity->onCollision(this);
            }
        }
    }
}

bool LivingEntity::isImmobile() const
{
    return health <= 0;
}

void LivingEntity::jump()
{
    velocityY = 0.42;
}

bool LivingEntity::canDespawn() const
{
    return true;
}

void LivingEntity::tryDespawn()
{
    if (world == nullptr || !canDespawn()) {
        return;
    }
    Entity* closest = world->getClosestPlayerEntity(this, -1.0);
    if (closest == nullptr) {
        return;
    }
    const double dx = closest->x - x;
    const double dy = closest->y - y;
    const double dz = closest->z - z;
    const double distSq = dx * dx + dy * dy + dz * dz;
    if (distSq > 16384.0) {
        markDead();
        return;
    }
    if (despawnCounter > 600 && random.nextInt(800) == 0) {
        if (distSq < 1024.0) {
            despawnCounter = 0;
        } else {
            markDead();
        }
    }
}

void LivingEntity::tickLiving()
{
    ++despawnCounter;
    tryDespawn();
    sidewaysSpeed = 0.0f;
    forwardSpeed = 0.0f;
    constexpr float lookRange = 8.0f;
    if (random.nextFloat() < 0.02f) {
        Entity* nearby = world != nullptr ? world->getClosestPlayerEntity(this, lookRange) : nullptr;
        if (nearby != nullptr) {
            lookTarget = nearby;
            lookTimer = 10 + random.nextInt(20);
        } else {
            rotationSpeed = (random.nextFloat() - 0.5f) * 20.0f;
        }
    }
    if (lookTarget != nullptr) {
        lookAt(lookTarget, 10.0f, getMaxLookPitchChange());
        if (--lookTimer <= 0 || lookTarget->dead || lookTarget->getSquaredDistance(*this) > static_cast<double>(lookRange * lookRange)) {
            lookTarget = nullptr;
        }
    } else {
        if (random.nextFloat() < 0.05f) {
            rotationSpeed = (random.nextFloat() - 0.5f) * 20.0f;
        }
        yaw += rotationSpeed;
        pitch = defaultPitch;
    }
    const bool inWater = isSubmergedInWater();
    const bool inLava = isTouchingLava();
    if (inWater || inLava) {
        jumping = random.nextFloat() < 0.8f;
    }
}

int LivingEntity::getMaxLookPitchChange() const
{
    return 40;
}

void LivingEntity::lookAt(Entity* target, float maxPitch, float maxYaw)
{
    if (target == nullptr) {
        return;
    }
    const double dx = target->x - x;
    const double dz = target->z - z;
    double dy = 0.0;
    if (const auto* livingTarget = dynamic_cast<LivingEntity*>(target); livingTarget != nullptr) {
        dy = y + static_cast<double>(getEyeHeight()) - (livingTarget->y + static_cast<double>(livingTarget->getEyeHeight()));
    } else {
        dy = (target->boundingBox.minY + target->boundingBox.maxY) * 0.5 - (y + static_cast<double>(getEyeHeight()));
    }
    const double flatDistance = MathHelper::sqrt(dx * dx + dz * dz);
    const float targetYaw = static_cast<float>(std::atan2(dz, dx) * 180.0 / kPiF) - 90.0f;
    const float targetPitch = static_cast<float>(-(std::atan2(dy, flatDistance) * 180.0 / kPiF));
    pitch = -lerpRotation(pitch, targetPitch, maxYaw);
    yaw = lerpRotation(yaw, targetYaw, maxPitch);
}

bool LivingEntity::hasLookTarget() const
{
    return lookTarget != nullptr;
}

Entity* LivingEntity::getLookTarget() const
{
    return lookTarget;
}

void LivingEntity::beforeRemove()
{
}

bool LivingEntity::canSpawn() const
{
    if (world == nullptr) {
        return false;
    }
    return world->canSpawnEntity(boundingBox)
        && world->getEntityCollisions(const_cast<LivingEntity*>(this), boundingBox).empty()
        && !world->isBoxSubmergedInFluid(boundingBox);
}

void LivingEntity::tickInVoid()
{
    damage(nullptr, 4);
}

Vec3d LivingEntity::getPosition(float tickDelta) const
{
    if (tickDelta == 1.0f) {
        return {x, y, z};
    }
    return {
        prevX + (x - prevX) * static_cast<double>(tickDelta),
        prevY + (y - prevY) * static_cast<double>(tickDelta),
        prevZ + (z - prevZ) * static_cast<double>(tickDelta)};
}

Vec3d LivingEntity::getLookVector(float tickDelta) const
{
    if (tickDelta == 1.0f) {
        return Entity::getLookVector();
    }
    const float interpolatedPitch = prevPitch + (pitch - prevPitch) * tickDelta;
    const float interpolatedYaw = prevYaw + (yaw - prevYaw) * tickDelta;
    const float cosYaw = MathHelper::cos(-interpolatedYaw * (kPiF / 180.0f) - kPiF);
    const float sinYaw = MathHelper::sin(-interpolatedYaw * (kPiF / 180.0f) - kPiF);
    const float cosPitch = -MathHelper::cos(-interpolatedPitch * (kPiF / 180.0f));
    const float sinPitch = MathHelper::sin(-interpolatedPitch * (kPiF / 180.0f));
    return {sinYaw * cosPitch, sinPitch, cosYaw * cosPitch};
}

Vec3d LivingEntity::getLookVector() const
{
    return getLookVector(1.0f);
}

int LivingEntity::getLimitPerChunk() const
{
    return 4;
}

ItemStack LivingEntity::getHeldItem() const
{
    return {};
}

void LivingEntity::processServerEntityStatus(std::int8_t status)
{
    if (status == 2) {
        walkAnimationSpeed = 1.5f;
        hearts = maxHealth;
        damagedTime = 10;
        hurtTime = 10;
        damagedSwingDir = 0.0f;
        if (world != nullptr) {
            const std::string hurtSound = getHurtSound();
            if (!hurtSound.empty()) {
                world->playSound(this, hurtSound, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
            }
        }
        damage(nullptr, 0);
        return;
    }
    if (status == 3) {
        if (world != nullptr) {
            const std::string deathSound = getDeathSound();
            if (!deathSound.empty()) {
                world->playSound(this, deathSound, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
            }
        }
        health = 0;
        onKilledBy(nullptr);
        return;
    }
    Entity::processServerEntityStatus(status);
}

bool LivingEntity::isSleeping() const
{
    return false;
}

int LivingEntity::getItemStackTextureId(const ItemStack& stack) const
{
    return stack.getTextureId();
}

float LivingEntity::getHandSwingProgress(float tickDelta) const
{
    float delta = swingAnimationProgress - lastSwingAnimationProgress;
    if (delta < 0.0f) {
        delta += 1.0f;
    }
    return lastSwingAnimationProgress + delta * tickDelta;
}

float LivingEntity::lerpRotation(float from, float to, float maxChange) const
{
    float delta = to - from;
    while (delta < -180.0f) {
        delta += 360.0f;
    }
    while (delta >= 180.0f) {
        delta -= 360.0f;
    }
    delta = std::clamp(delta, -maxChange, maxChange);
    return from + delta;
}

} // namespace net::minecraft::entity

void LivingEntity::registerClass()
{
    ::net::minecraft::entity::detail::registerVanillaEntity<LivingEntity>("Mob", 48);
}

static ::net::minecraft::registry::RegisterEntity<LivingEntity> autoReg(48);
