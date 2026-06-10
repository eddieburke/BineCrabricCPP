#pragma once

#include "net/minecraft/entity/mob/MonsterEntity.hpp"

namespace net::minecraft::entity::mob {

class ZombieEntity : public MonsterEntity {
public:
    static void registerClass();
    explicit ZombieEntity(World* world = nullptr);

    void tickMovement() override;

    [[nodiscard]] std::string getRandomSound() override { return "mob.zombie"; }
    [[nodiscard]] std::string getHurtSound() const override { return "mob.zombiehurt"; }
    [[nodiscard]] std::string getDeathSound() const override { return "mob.zombiedeath"; }

    [[nodiscard]] int getDroppedItemId() const override;
};

} // namespace net::minecraft::entity::mob
