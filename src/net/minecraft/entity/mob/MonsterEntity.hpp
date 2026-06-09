#pragma once

#include "net/minecraft/entity/MobEntity.hpp"
#include "net/minecraft/entity/Monster.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/LightType.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity::player {
class PlayerEntity;
}

namespace net::minecraft::entity::mob {

class MonsterEntity : public MobEntity, public Monster {
public:
    explicit MonsterEntity(World* world = nullptr) : MobEntity(world)
    {
        health = 20;
    }

    int attackDamage = 2;

    void tick() override;
    void tickMovement() override;
    bool damage(Entity* damageSource, int amount) override;

    [[nodiscard]] bool canSpawn() const override
    {
        if (world == nullptr) {
            return false;
        }
        const int bx = MathHelper::floor(x);
        const int by = MathHelper::floor(boundingBox.minY);
        const int bz = MathHelper::floor(z);
        if (world->getBrightness(LightType::Sky, bx, by, bz) > world->random().nextInt(32)) {
            return false;
        }
        int light = world->getLightLevel(bx, by, bz);
        if (world->isThundering()) {
            const int savedAmbient = world->ambientDarkness;
            world->ambientDarkness = 10;
            light = world->getLightLevel(bx, by, bz);
            world->ambientDarkness = savedAmbient;
        }
        return light <= world->random().nextInt(8) && MobEntity::canSpawn();
    }

protected:
    Entity* getTargetInRange() override;
    void attack(Entity* other, float distance) override;
    float getPathfindingFavor(int x, int y, int z) const override;
};

} // namespace net::minecraft::entity::mob
