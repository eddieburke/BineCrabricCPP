#pragma once

#include "net/minecraft/entity/FlyingEntity.hpp"
#include "net/minecraft/entity/Monster.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::entity::mob {

class GhastEntity : public FlyingEntity, public Monster {
public:
    explicit GhastEntity(World* world = nullptr);

    int floatDuration = 0;
    double targetX = 0.0;
    double targetY = 0.0;
    double targetZ = 0.0;
    int lastChargeTime = 0;
    int chargeTime = 0;

    void initDataTracker() override
    {
        LivingEntity::initDataTracker();
        dataTracker.startTracking(16, static_cast<std::int8_t>(0));
    }

    void tick() override;

    [[nodiscard]] std::string getRandomSound() override { return "mob.ghast.moan"; }
    [[nodiscard]] std::string getHurtSound() const override { return "mob.ghast.scream"; }
    [[nodiscard]] std::string getDeathSound() const override { return "mob.ghast.death"; }
    [[nodiscard]] float getSoundVolume() const override { return 10.0f; }

    [[nodiscard]] int getDroppedItemId() const override
    {
        return Item::GUNPOWDER != nullptr ? Item::GUNPOWDER->id : 289;
    }

    [[nodiscard]] bool canSpawn() const override;
    [[nodiscard]] int getLimitPerChunk() const override { return 1; }

protected:
    void tickLiving() override;

private:
    Entity* ghastTarget = nullptr;
    int angerCooldown = 0;
    bool canReach(double x, double y, double z, double steps) const;
};

} // namespace net::minecraft::entity::mob
