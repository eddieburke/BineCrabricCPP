#pragma once

#include "net/minecraft/entity/mob/MonsterEntity.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::entity::mob {

class SpiderEntity : public MonsterEntity {
public:
    explicit SpiderEntity(World* world = nullptr);

    [[nodiscard]] double getPassengerRidingHeight() const override
    {
        return static_cast<double>(height) * 0.75 - 0.5;
    }

    [[nodiscard]] bool isOnLadder() const override { return horizontalCollision; }

    [[nodiscard]] bool bypassesSteppingEffects() const override { return false; }

protected:
    Entity* getTargetInRange() override;
    void attack(Entity* other, float distance) override;

    [[nodiscard]] std::string getRandomSound() override { return "mob.spider"; }
    [[nodiscard]] std::string getHurtSound() const override { return "mob.spider"; }
    [[nodiscard]] std::string getDeathSound() const override { return "mob.spiderdeath"; }

    [[nodiscard]] int getDroppedItemId() const override
    {
        return Item::STRING != nullptr ? Item::STRING->id : 287;
    }
};

} // namespace net::minecraft::entity::mob
