#include "net/minecraft/entity/player/PlayerEntity.hpp"

#include "net/minecraft/achievement/Achievements.hpp"
#include "net/minecraft/block/BedBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/mob/MonsterEntity.hpp"
#include "net/minecraft/entity/passive/PigEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"

#include <cmath>
#include <vector>

namespace net::minecraft::entity::player {
namespace {

bool isHostileMobDamageSource(const Entity& source)
{
    if (dynamic_cast<const entity::mob::MonsterEntity*>(&source) != nullptr) {
        return true;
    }
    const std::string id = EntityRegistry::getId(source);
    return id == "Monster" || id == "Creeper" || id == "Skeleton" || id == "Spider"
        || id == "Giant" || id == "Zombie" || id == "Slime" || id == "Ghast" || id == "PigZombie";
}

bool isArrowDamageSource(const Entity& source)
{
    if (dynamic_cast<const entity::projectile::ArrowEntity*>(&source) != nullptr) {
        return true;
    }
    return EntityRegistry::getId(source) == "Arrow";
}

} // namespace

PlayerEntity::PlayerEntity(World* world)
    : LivingEntity(world),
      inventory(this),
      playerScreenHandler(&inventory, world == nullptr || !world->isRemote()),
      currentScreenHandler(&playerScreenHandler)
{
    initDataTracker();
    standingEyeHeight = 1.62f;
    if (world != nullptr) {
        const Vec3i spawn = world->getSpawnPos();
        setPositionAndAnglesKeepPrevAngles(
            static_cast<double>(spawn.x) + 0.5,
            static_cast<double>(spawn.y) + 1.0,
            static_cast<double>(spawn.z) + 0.5,
            0.0f,
            0.0f);
    }
    health = 20;
    maxHealth = 20;
    modelName = "humanoid";
    rotationOffset = 180.0f;
    fireImmunityTicks = 20;
    texture = "/mob/char.png";
}

void PlayerEntity::initDataTracker()
{
    LivingEntity::initDataTracker();
    dataTracker.startTracking(16, static_cast<std::int8_t>(0));
}

void PlayerEntity::tick()
{
    if (isSleeping()) {
        ++sleepTimer;
        if (sleepTimer > 100) {
            sleepTimer = 100;
        }
        if (world != nullptr && !world->isRemote()) {
            if (!isSleepingInBed()) {
                wakeUp(true, true, false);
            } else if (world->canMonsterSpawn()) {
                wakeUp(false, true, true);
            }
        }
    } else if (sleepTimer > 0) {
        ++sleepTimer;
        if (sleepTimer >= 110) {
            sleepTimer = 0;
        }
    }
    LivingEntity::tick();
    if (world != nullptr && !world->isRemote() && currentScreenHandler != nullptr
        && !currentScreenHandler->canUse(this)) {
        closeHandledScreen();
        currentScreenHandler = &playerScreenHandler;
    }
    prevCapeX = capeX;
    prevCapeY = capeY;
    prevCapeZ = capeZ;
    const double dx = x - capeX;
    const double dy = y - capeY;
    const double dz = z - capeZ;
    constexpr double maxDelta = 10.0;
    if (dx > maxDelta || dx < -maxDelta) {
        prevCapeX = capeX = x;
    }
    if (dz > maxDelta || dz < -maxDelta) {
        prevCapeZ = capeZ = z;
    }
    if (dy > maxDelta || dy < -maxDelta) {
        prevCapeY = capeY = y;
    }
    capeX += dx * 0.25;
    capeY += dy * 0.25;
    capeZ += dz * 0.25;
    increaseStat(stat::Stats::PLAY_ONE_MINUTE.id, 1);
    if (vehicle == nullptr) {
        ridingStartPos.reset();
    }
}

bool PlayerEntity::isImmobile() const
{
    return health <= 0 || isSleeping();
}

void PlayerEntity::closeHandledScreen()
{
}

void PlayerEntity::tickRiding()
{
    const double startX = x;
    const double startY = y;
    const double startZ = z;
    LivingEntity::tickRiding();
    prevStepBobbingAmount = stepBobbingAmount;
    stepBobbingAmount = 0.0f;
    increaseRidingMotionStats(x - startX, y - startY, z - startZ);
}

void PlayerEntity::teleportTop()
{
    standingEyeHeight = 1.62f;
    setBoundingBoxSpacing(0.6f, 1.8f);
    Entity::teleportTop();
    health = 20;
    deathTime = 0;
}

void PlayerEntity::tickLiving()
{
    if (handSwinging) {
        ++handSwingTicks;
        if (handSwingTicks >= 8) {
            handSwingTicks = 0;
            handSwinging = false;
        }
    } else {
        handSwingTicks = 0;
    }
    swingAnimationProgress = static_cast<float>(handSwingTicks) / 8.0f;
}

void PlayerEntity::tickMovement()
{
    if (world != nullptr && world->difficulty == 0 && health < 20 && age % (20 * 12) == 0) {
        heal(1);
    }
    inventory.inventoryTick();
    prevStepBobbingAmount = stepBobbingAmount;
    LivingEntity::tickMovement();
    float speed = MathHelper::sqrt(static_cast<float>(velocityX * velocityX + velocityZ * velocityZ));
    float bob = static_cast<float>(std::atan(-velocityY * 0.2) * 15.0);
    if (speed > 0.1f) {
        speed = 0.1f;
    }
    if (!onGround || health <= 0) {
        speed = 0.0f;
    }
    if (onGround || health <= 0) {
        bob = 0.0f;
    }
    stepBobbingAmount += (speed - stepBobbingAmount) * 0.4f;
    tilt += (bob - tilt) * 0.8f;
    if (health > 0 && world != nullptr) {
        const std::vector<Entity*> nearby = world->getEntities(this, boundingBox.expand(1.0));
        for (Entity* entity : nearby) {
            if (entity != nullptr && !entity->dead) {
                collideWithEntity(entity);
            }
        }
    }
}

void PlayerEntity::collideWithEntity(Entity* entity)
{
    if (entity != nullptr) {
        entity->onPlayerInteraction(this);
    }
}

void PlayerEntity::onKilledBy(Entity* adversary)
{
    LivingEntity::onKilledBy(adversary);
    setBoundingBoxSpacing(0.2f, 0.2f);
    setPosition(x, y, z);
    velocityY = 0.1;
    inventory.dropInventory();
    if (adversary != nullptr) {
        velocityX = -MathHelper::cos((damagedSwingDir + yaw) * (kPiF / 180.0f)) * 0.1;
        velocityZ = -MathHelper::sin((damagedSwingDir + yaw) * (kPiF / 180.0f)) * 0.1;
    } else {
        velocityX = 0.0;
        velocityZ = 0.0;
    }
    standingEyeHeight = 0.1f;
    increaseStat(stat::Stats::DEATHS.id, 1);
}

void PlayerEntity::updateKilledAchievement(LivingEntity* entityKilled, int scoreIn)
{
    score += scoreIn;
    if (entityKilled == nullptr) {
        return;
    }
    if (dynamic_cast<PlayerEntity*>(entityKilled) != nullptr) {
        increaseStat(stat::Stats::PLAYER_KILLS.id, 1);
    } else {
        increaseStat(stat::Stats::MOB_KILLS.id, 1);
    }
}

void PlayerEntity::dropSelectedItem()
{
    dropItem(inventory.removeStack(static_cast<std::size_t>(inventory.selectedSlot), 1), false);
}

void PlayerEntity::dropItem(ItemStack stack)
{
    dropItem(std::move(stack), false);
}

void PlayerEntity::dropItem(ItemStack stack, bool throwRandomly)
{
    if (stack.empty() || world == nullptr) {
        return;
    }
    const double dropY = y - 0.3 + static_cast<double>(getEyeHeight());
    auto* entity = new ItemEntity(world, x, dropY, z, std::move(stack));
    entity->pickupDelay = 40;
    float impulse = 0.1f;
    if (throwRandomly) {
        const float spread = random.nextFloat() * 0.5f;
        const float angle = random.nextFloat() * kPiF * 2.0f;
        entity->velocityX = -MathHelper::sin(angle) * static_cast<double>(spread);
        entity->velocityZ = MathHelper::cos(angle) * static_cast<double>(spread);
        entity->velocityY = 0.2;
    } else {
        impulse = 0.3f;
        entity->velocityX = -MathHelper::sin(yaw / 180.0f * kPiF) * MathHelper::cos(pitch / 180.0f * kPiF) * impulse;
        entity->velocityZ = MathHelper::cos(yaw / 180.0f * kPiF) * MathHelper::cos(pitch / 180.0f * kPiF) * impulse;
        entity->velocityY = -MathHelper::sin(pitch / 180.0f * kPiF) * impulse + 0.1f;
        impulse = 0.02f;
        const float angle = random.nextFloat() * kPiF * 2.0f;
        entity->velocityX += MathHelper::cos(angle) * static_cast<double>(impulse * random.nextFloat());
        entity->velocityY += static_cast<double>((random.nextFloat() - random.nextFloat()) * 0.1f);
        entity->velocityZ += MathHelper::sin(angle) * static_cast<double>(impulse * random.nextFloat());
    }
    world->spawnEntity(entity);
    increaseStat(stat::Stats::DROP.id, 1);
}

float PlayerEntity::getBlockBreakingSpeed(int blockId) const
{
    float speed = inventory.getStrengthOnBlock(blockId);
    if (isInFluid(block::material::Material::WATER)) {
        speed /= 5.0f;
    }
    if (!onGround) {
        speed /= 5.0f;
    }
    return speed;
}

bool PlayerEntity::canHarvest(int blockId) const
{
    return inventory.isUsingEffectiveTool(blockId);
}

void PlayerEntity::readNbt(const NbtCompound& nbt)
{
    LivingEntity::readNbt(nbt);
    if (nbt.contains("Inventory")) {
        inventory.readNbt(nbt.getList("Inventory"));
    }
    dimensionId = nbt.getInt("Dimension");
    sleeping = nbt.getBoolean("Sleeping");
    sleepTimer = nbt.getShort("SleepTimer");
    if (sleeping) {
        sleepingPos = Vec3i(MathHelper::floor(x), MathHelper::floor(y), MathHelper::floor(z));
        wakeUp(true, true, false);
    }
    if (nbt.contains("SpawnX") && nbt.contains("SpawnY") && nbt.contains("SpawnZ")) {
        spawnPos = Vec3i(nbt.getInt("SpawnX"), nbt.getInt("SpawnY"), nbt.getInt("SpawnZ"));
    } else {
        spawnPos.reset();
    }
}

void PlayerEntity::writeNbt(NbtCompound& nbt) const
{
    LivingEntity::writeNbt(nbt);
    nbt.put("Inventory", inventory.writeNbt());
    nbt.putInt("Dimension", dimensionId);
    nbt.putBoolean("Sleeping", sleeping);
    nbt.putShort("SleepTimer", static_cast<std::int16_t>(sleepTimer));
    if (spawnPos.has_value()) {
        nbt.putInt("SpawnX", spawnPos->x);
        nbt.putInt("SpawnY", spawnPos->y);
        nbt.putInt("SpawnZ", spawnPos->z);
    }
}

void PlayerEntity::openChestScreen(Inventory* inventoryIn)
{
    (void)inventoryIn;
}

void PlayerEntity::openChestScreen(int xIn, int yIn, int zIn)
{
    (void)xIn;
    (void)yIn;
    (void)zIn;
}

void PlayerEntity::openFurnaceScreen(::net::minecraft::block::entity::FurnaceBlockEntity* furnaceIn)
{
    (void)furnaceIn;
}

void PlayerEntity::openDispenserScreen(::net::minecraft::block::entity::DispenserBlockEntity* dispenserIn)
{
    (void)dispenserIn;
}

void PlayerEntity::openCraftingScreen(int xIn, int yIn, int zIn)
{
    (void)xIn;
    (void)yIn;
    (void)zIn;
}

float PlayerEntity::getEyeHeight() const
{
    return 0.12f;
}

void PlayerEntity::resetEyeHeight()
{
    standingEyeHeight = 1.62f;
}

bool PlayerEntity::damage(Entity* damageSource, int amount)
{
    despawnCounter = 0;
    if (health <= 0) {
        return false;
    }
    if (isSleeping() && world != nullptr && !world->isRemote()) {
        wakeUp(true, true, false);
    }
    if (damageSource != nullptr && world != nullptr) {
        if (isHostileMobDamageSource(*damageSource) || isArrowDamageSource(*damageSource)) {
            if (world->difficulty == 0) {
                amount = 0;
            } else if (world->difficulty == 1) {
                amount = amount / 3 + 1;
            } else if (world->difficulty == 3) {
                amount = amount * 3 / 2;
            }
        }
    }
    if (amount == 0) {
        return false;
    }
    increaseStat(stat::Stats::DAMAGE_TAKEN.id, amount);
    return LivingEntity::damage(damageSource, amount);
}

bool PlayerEntity::isPvpEnabled() const
{
    return false;
}

void PlayerEntity::applyDamage(int amount)
{
    const int armor = 25 - inventory.getTotalArmorDurability();
    const int total = amount * armor + damageSpill;
    inventory.damageArmor(amount);
    amount = total / 25;
    damageSpill = total % 25;
    LivingEntity::applyDamage(amount);
}

ItemStack PlayerEntity::getHand() const
{
    const ItemStack* stack = inventory.getSelectedItem();
    return stack == nullptr ? ItemStack() : *stack;
}

void PlayerEntity::clearStackInHand()
{
    inventory.setStack(static_cast<std::size_t>(inventory.selectedSlot), {});
}

double PlayerEntity::getStandingEyeHeight() const
{
    return static_cast<double>(standingEyeHeight - 0.5f);
}

void PlayerEntity::swingHand()
{
    handSwingTicks = -1;
    handSwinging = true;
}

void PlayerEntity::attack(Entity* target)
{
    if (target == nullptr) {
        return;
    }
    int amount = inventory.getAttackDamage(target);
    if (amount <= 0) {
        return;
    }
    if (velocityY < 0.0) {
        ++amount;
    }
    target->damage(this, amount);
    ItemStack* handStack = inventory.getSelectedItem();
    if (handStack != nullptr && !handStack->empty() && handStack->getItem() != nullptr) {
        LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target);
        if (livingTarget != nullptr) {
            handStack->postHit(livingTarget, this);
            if (handStack->count <= 0) {
                handStack->onRemoved(this);
                clearStackInHand();
            }
        }
    }
    if (LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target); livingTarget != nullptr) {
        if (livingTarget->isAlive()) {
            // commandWolvesToAttack stub: wolf pack attack not yet ported
        }
        increaseStat(stat::Stats::DAMAGE_DEALT.id, amount);
    }
}

void PlayerEntity::interact(Entity* entity)
{
    if (entity == nullptr) {
        return;
    }
    if (entity->interact(this)) {
        return;
    }
    ItemStack* handStack = inventory.getSelectedItem();
    if (handStack != nullptr && !handStack->empty() && handStack->getItem() != nullptr) {
        LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(entity);
        if (livingTarget != nullptr) {
            handStack->getItem()->useOnEntity(handStack, livingTarget);
            if (handStack->count <= 0) {
                handStack->onRemoved(this);
                clearStackInHand();
            }
        }
    }
}

void PlayerEntity::respawn()
{
}

void PlayerEntity::spawn()
{
}

void PlayerEntity::onCursorStackChanged(const ItemStack& stack)
{
    (void)stack;
}

void PlayerEntity::markDead()
{
    Entity::markDead();
}

bool PlayerEntity::isInsideWall() const
{
    return !sleeping && Entity::isInsideWall();
}

SleepAttemptResult PlayerEntity::trySleep(int xIn, int yIn, int zIn)
{
    if (world != nullptr && !world->isRemote()) {
        if (isSleeping() || health <= 0) {
            return SleepAttemptResult::NotPossible;
        }
        if (world->dimension != nullptr && world->dimension->isNether) {
            return SleepAttemptResult::WrongDimension;
        }
        if (world->canMonsterSpawn()) {
            return SleepAttemptResult::MonstersNearby;
        }
        if (std::abs(x - static_cast<double>(xIn)) > 3.0 || std::abs(y - static_cast<double>(yIn)) > 2.0
            || std::abs(z - static_cast<double>(zIn)) > 3.0) {
            return SleepAttemptResult::TooFarAway;
        }
    }
    setBoundingBoxSpacing(0.2f, 0.2f);
    standingEyeHeight = 0.2f;
    if (world != nullptr && world->isPosLoaded(xIn, yIn, zIn)) {
        const int meta = world->getBlockMeta(xIn, yIn, zIn);
        const int direction = block::BedBlock::getDirection(meta);
        float offsetX = 0.5f;
        float offsetZ = 0.5f;
        switch (direction) {
        case 0:
            offsetZ = 0.9f;
            break;
        case 2:
            offsetZ = 0.1f;
            break;
        case 1:
            offsetX = 0.1f;
            break;
        case 3:
            offsetX = 0.9f;
            break;
        default:
            break;
        }
        calculateSleepOffset(direction);
        setPosition(static_cast<float>(xIn) + offsetX, static_cast<float>(yIn) + 0.9375f, static_cast<float>(zIn) + offsetZ);
    } else {
        setPosition(static_cast<float>(xIn) + 0.5f, static_cast<float>(yIn) + 0.9375f, static_cast<float>(zIn) + 0.5f);
    }
    sleeping = true;
    sleepTimer = 0;
    sleepingPos = Vec3i(xIn, yIn, zIn);
    velocityX = 0.0;
    velocityY = 0.0;
    velocityZ = 0.0;
    if (world != nullptr && !world->isRemote()) {
        world->updateSleepingPlayers();
    }
    return SleepAttemptResult::Ok;
}

void PlayerEntity::wakeUp(bool resetSleepTimer, bool updateSleepingPlayers, bool setSpawnPosFlag)
{
    setBoundingBoxSpacing(0.6f, 1.8f);
    resetEyeHeight();
    if (sleepingPos.has_value() && world != nullptr) {
        const Vec3i bedPos = *sleepingPos;
        std::optional<Vec3i> wakePos;
        if (Block::BED != nullptr && world->getBlockId(bedPos.x, bedPos.y, bedPos.z) == Block::BED->id) {
            wakePos = block::BedBlock::findWakeUpPosition(world, bedPos.x, bedPos.y, bedPos.z, 0);
        }
        if (!wakePos.has_value()) {
            wakePos = Vec3i(bedPos.x, bedPos.y + 1, bedPos.z);
        }
        setPosition(
            static_cast<float>(wakePos->x) + 0.5f,
            static_cast<float>(wakePos->y) + standingEyeHeight + 0.1f,
            static_cast<float>(wakePos->z) + 0.5f);
    }
    sleeping = false;
    if (world != nullptr && !world->isRemote() && updateSleepingPlayers) {
        world->updateSleepingPlayers();
    }
    sleepTimer = resetSleepTimer ? 0 : 100;
    if (setSpawnPosFlag) {
        setSpawnPos(sleepingPos);
    }
}

bool PlayerEntity::isSleeping() const
{
    return sleeping;
}

float PlayerEntity::getSleepingRotation() const
{
    if (!sleepingPos.has_value() || world == nullptr || Block::BED == nullptr) {
        return 0.0f;
    }
    const Vec3i& bedPos = *sleepingPos;
    const int direction = block::BedBlock::getDirection(world->getBlockMeta(bedPos.x, bedPos.y, bedPos.z));
    switch (direction) {
    case 0:
        return 90.0f;
    case 1:
        return 0.0f;
    case 2:
        return 270.0f;
    case 3:
        return 180.0f;
    default:
        return 0.0f;
    }
}

bool PlayerEntity::isSleepingInBed() const
{
    if (!sleepingPos.has_value() || world == nullptr || Block::BED == nullptr) {
        return false;
    }
    const Vec3i& bedPos = *sleepingPos;
    return world->getBlockId(bedPos.x, bedPos.y, bedPos.z) == Block::BED->id;
}

void PlayerEntity::updateCapeUrl()
{
    playerCapeUrl = "http://s3.amazonaws.com/MinecraftCloaks/" + name + ".png";
    capeUrl = playerCapeUrl;
}

bool PlayerEntity::isFullyAsleep() const
{
    return sleeping && sleepTimer >= 100;
}

int PlayerEntity::getSleepTimer() const
{
    return sleepTimer;
}

void PlayerEntity::sendMessage(const std::string& message)
{
    (void)message;
}

std::optional<Vec3i> PlayerEntity::getSpawnPos() const
{
    return spawnPos;
}

void PlayerEntity::setSpawnPos(std::optional<Vec3i> spawnPosIn)
{
    spawnPos = spawnPosIn;
}

std::optional<Vec3i> PlayerEntity::findRespawnPosition(World* world, const Vec3i& spawnPosIn)
{
    if (world == nullptr) {
        return std::nullopt;
    }
    if (ChunkSource* chunkSource = world->getChunkSource()) {
        chunkSource->loadChunk((spawnPosIn.x - 3) >> 4, (spawnPosIn.z - 3) >> 4);
        chunkSource->loadChunk((spawnPosIn.x + 3) >> 4, (spawnPosIn.z - 3) >> 4);
        chunkSource->loadChunk((spawnPosIn.x - 3) >> 4, (spawnPosIn.z + 3) >> 4);
        chunkSource->loadChunk((spawnPosIn.x + 3) >> 4, (spawnPosIn.z + 3) >> 4);
    }
    if (Block::BED == nullptr || world->getBlockId(spawnPosIn.x, spawnPosIn.y, spawnPosIn.z) != Block::BED->id) {
        return std::nullopt;
    }
    return block::BedBlock::findWakeUpPosition(world, spawnPosIn.x, spawnPosIn.y, spawnPosIn.z, 0);
}

void PlayerEntity::increaseStat(int stat, int amount)
{
    (void)stat;
    (void)amount;
}

void PlayerEntity::incrementStat(int stat)
{
    increaseStat(stat, 1);
}

void PlayerEntity::jump()
{
    LivingEntity::jump();
    increaseStat(stat::Stats::JUMP.id, 1);
}

void PlayerEntity::onLanding(float fallDistanceIn)
{
    if (fallDistanceIn >= 2.0f) {
        increaseStat(stat::Stats::FALL_ONE_CM.id, static_cast<int>(std::round(fallDistanceIn * 100.0)));
    }
    LivingEntity::onLanding(fallDistanceIn);
}

void PlayerEntity::travel(float sideways, float forward)
{
    const double startX = x;
    const double startY = y;
    const double startZ = z;
    LivingEntity::travel(sideways, forward);
    updateMovementStats(x - startX, y - startY, z - startZ);
}

void PlayerEntity::onKilledOther(LivingEntity* other)
{
    if (other != nullptr && dynamic_cast<entity::mob::MonsterEntity*>(other) != nullptr) {
        incrementStat(achievement::Achievements::KILL_ENEMY.statId());
    }
}

int PlayerEntity::getItemStackTextureId(const ItemStack& stack) const
{
    return stack.getTextureId();
}

void PlayerEntity::tickPortalCooldown()
{
    if (portalCooldown > 0) {
        portalCooldown = 10;
        return;
    }
    inTeleportationState = true;
}

void PlayerEntity::calculateSleepOffset(int bedDirection)
{
    sleepOffsetX = 0.0f;
    sleepOffsetZ = 0.0f;
    switch (bedDirection) {
    case 0:
        sleepOffsetZ = -1.8f;
        break;
    case 2:
        sleepOffsetZ = 1.8f;
        break;
    case 1:
        sleepOffsetX = 1.8f;
        break;
    case 3:
        sleepOffsetX = -1.8f;
        break;
    default:
        break;
    }
}

void PlayerEntity::updateMovementStats(double dx, double dy, double dz)
{
    if (vehicle != nullptr) {
        return;
    }
    if (isInFluid(block::material::Material::WATER)) {
        const int distance = static_cast<int>(std::round(
            MathHelper::sqrt(static_cast<float>(dx * dx + dy * dy + dz * dz)) * 100.0f));
        if (distance > 0) {
            increaseStat(stat::Stats::DIVE_ONE_CM.id, distance);
        }
    } else if (isSubmergedInWater()) {
        const int distance = static_cast<int>(std::round(
            MathHelper::sqrt(static_cast<float>(dx * dx + dz * dz)) * 100.0f));
        if (distance > 0) {
            increaseStat(stat::Stats::SWIM_ONE_CM.id, distance);
        }
    } else if (isOnLadder()) {
        if (dy > 0.0) {
            increaseStat(stat::Stats::CLIMB_ONE_CM.id, static_cast<int>(std::round(dy * 100.0)));
        }
    } else if (onGround) {
        const int distance = static_cast<int>(std::round(
            MathHelper::sqrt(static_cast<float>(dx * dx + dz * dz)) * 100.0f));
        if (distance > 0) {
            increaseStat(stat::Stats::WALK_ONE_CM.id, distance);
        }
    } else {
        const int distance = static_cast<int>(std::round(
            MathHelper::sqrt(static_cast<float>(dx * dx + dz * dz)) * 100.0f));
        if (distance > 25) {
            increaseStat(stat::Stats::FLY_ONE_CM.id, distance);
        }
    }
}

void PlayerEntity::increaseRidingMotionStats(double deltaX, double deltaY, double deltaZ)
{
    if (vehicle == nullptr) {
        return;
    }
    const int distance = static_cast<int>(std::round(
        MathHelper::sqrt(static_cast<float>(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ)) * 100.0f));
    if (distance <= 0) {
        return;
    }
    const std::string vehicleId = EntityRegistry::getId(*vehicle);
    if (vehicleId == "Minecart" || dynamic_cast<entity::vehicle::MinecartEntity*>(vehicle) != nullptr) {
        increaseStat(stat::Stats::MINECART_ONE_CM.id, distance);
        if (!ridingStartPos.has_value()) {
            ridingStartPos = Vec3i(MathHelper::floor(x), MathHelper::floor(y), MathHelper::floor(z));
        } else {
            const int startDx = ridingStartPos->x - MathHelper::floor(x);
            const int startDy = ridingStartPos->y - MathHelper::floor(y);
            const int startDz = ridingStartPos->z - MathHelper::floor(z);
            const double railDistance = MathHelper::sqrt(static_cast<float>(
                startDx * startDx + startDy * startDy + startDz * startDz));
            if (railDistance >= 1000.0) {
                incrementStat(achievement::Achievements::CRAFT_RAIL.statId());
            }
        }
    } else if (vehicleId == "Boat" || dynamic_cast<entity::vehicle::BoatEntity*>(vehicle) != nullptr) {
        increaseStat(stat::Stats::BOAT_ONE_CM.id, distance);
    } else if (vehicleId == "Pig" || dynamic_cast<entity::passive::PigEntity*>(vehicle) != nullptr) {
        increaseStat(stat::Stats::PIG_ONE_CM.id, distance);
    }
}

} // namespace net::minecraft::entity::player
