#pragma once

#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/entity/player/SleepAttemptResult.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/screen/PlayerScreenHandler.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <optional>
#include <string>

namespace net::minecraft {
class World;
}

namespace net::minecraft::entity::projectile {
class FishingBobberEntity;
}

namespace net::minecraft::block::entity {
class FurnaceBlockEntity;
class DispenserBlockEntity;
}

namespace net::minecraft::entity::player {

class PlayerEntity : public LivingEntity {
public:
    using Entity::interact;

    explicit PlayerEntity(World* world = nullptr);

    PlayerInventory inventory;
    screen::PlayerScreenHandler playerScreenHandler;
    screen::ScreenHandler* currentScreenHandler = nullptr;
    int score = 0;

    [[nodiscard]] int getScore() const noexcept { return score; }

    float prevStepBobbingAmount = 0.0f;
    float stepBobbingAmount = 0.0f;
    bool handSwinging = false;
    int handSwingTicks = 0;
    std::string name = "Player";
    int dimensionId = 0;
    std::string playerCapeUrl;
    double prevCapeX = 0.0;
    double prevCapeY = 0.0;
    double prevCapeZ = 0.0;
    double capeX = 0.0;
    double capeY = 0.0;
    double capeZ = 0.0;
    bool sleeping = false;
    std::optional<Vec3i> sleepingPos;
    int sleepTimer = 0;
    float sleepOffsetX = 0.0f;
    float sleepOffsetY = 0.0f;
    float sleepOffsetZ = 0.0f;
    std::optional<Vec3i> spawnPos;
    std::optional<Vec3i> ridingStartPos;
    int portalCooldown = 20;
    bool touchingPortal = false;
    projectile::FishingBobberEntity* fishHook = nullptr;
    bool inTeleportationState = false;
    float screenDistortion = 0.0f;
    float lastScreenDistortion = 0.0f;
    float changeDimensionCooldown = 0.0f;
    int damageSpill = 0;

    void initDataTracker() override;
    void tick() override;
    [[nodiscard]] bool isImmobile() const override;
    virtual void closeHandledScreen();
    void tickRiding() override;
    void teleportTop() override;
    void tickLiving() override;
    void tickMovement() override;
    virtual void collideWithEntity(Entity* entity);
    void onKilledBy(Entity* adversary) override;
    void updateKilledAchievement(LivingEntity* entityKilled, int scoreIn) override;
    void dropSelectedItem();
    void dropItem(ItemStack stack);
    void dropItem(ItemStack stack, bool throwRandomly);
    [[nodiscard]] virtual float getBlockBreakingSpeed(int blockId) const;
    [[nodiscard]] virtual bool canHarvest(int blockId) const;
    void readNbt(const NbtCompound& nbt) override;
    void writeNbt(NbtCompound& nbt) const override;
    virtual void openChestScreen(Inventory* inventoryIn);
    virtual void openChestScreen(int xIn, int yIn, int zIn);
    virtual void openFurnaceScreen(::net::minecraft::block::entity::FurnaceBlockEntity* furnaceIn);
    virtual void openDispenserScreen(::net::minecraft::block::entity::DispenserBlockEntity* dispenserIn);
    virtual void openCraftingScreen(int xIn, int yIn, int zIn);
    [[nodiscard]] float getEyeHeight() const override;
    void resetEyeHeight();
    bool damage(Entity* damageSource, int amount) override;
    [[nodiscard]] virtual bool isPvpEnabled() const;
    void applyDamage(int amount) override;
    [[nodiscard]] ItemStack getHand() const;
    void clearStackInHand();
    [[nodiscard]] double getStandingEyeHeight() const;
    void swingHand();
    void attack(Entity* target);
    void interact(Entity* entity);
    virtual void respawn();
    virtual void spawn();
    virtual void onCursorStackChanged(const ItemStack& stack);
    void markDead();
    [[nodiscard]] bool isInsideWall() const override;
    virtual SleepAttemptResult trySleep(int xIn, int yIn, int zIn);
    virtual void wakeUp(bool resetSleepTimer, bool updateSleepingPlayers, bool setSpawnPosFlag);
    [[nodiscard]] bool isSleeping() const override;
    [[nodiscard]] float getSleepingRotation() const;
    [[nodiscard]] bool isSleepingInBed() const;
    void updateCapeUrl() override;
    [[nodiscard]] bool isFullyAsleep() const;
    [[nodiscard]] int getSleepTimer() const;
    virtual void sendMessage(const std::string& message);
    [[nodiscard]] std::optional<Vec3i> getSpawnPos() const;
    void setSpawnPos(std::optional<Vec3i> spawnPosIn);
    static std::optional<Vec3i> findRespawnPosition(World* world, const Vec3i& spawnPosIn);
    virtual void increaseStat(int stat, int amount);
    void incrementStat(int stat);
    void jump() override;
    void onLanding(float fallDistanceIn) override;
    void travel(float sideways, float forward) override;
    void onKilledOther(LivingEntity* other) override;
    [[nodiscard]] int getItemStackTextureId(const ItemStack& stack) const override;
    void tickPortalCooldown() override;

private:
    void calculateSleepOffset(int bedDirection);
    void updateMovementStats(double dx, double dy, double dz);
    void increaseRidingMotionStats(double deltaX, double deltaY, double deltaZ);
};

} // namespace net::minecraft::entity::player
