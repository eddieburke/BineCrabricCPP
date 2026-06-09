#pragma once

#include "net/minecraft/entity/MobEntity.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::entity {

class WaterCreatureEntity : public MobEntity {
public:
    explicit WaterCreatureEntity(World* world = nullptr) : MobEntity(world) {}

    [[nodiscard]] bool canBreatheInWater() const override { return true; }

    [[nodiscard]] bool canSpawn() const override
    {
        return world && world->canSpawnEntity(boundingBox);
    }

    [[nodiscard]] int getMinAmbientSoundDelay() const override { return 120; }
};

} // namespace net::minecraft::entity
