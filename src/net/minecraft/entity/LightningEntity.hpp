#pragma once

#include "net/minecraft/entity/AbstractLightningEntity.hpp"

namespace net::minecraft::entity {

class LightningEntity : public AbstractLightningEntity {
public:
    explicit LightningEntity(World* world = nullptr);
    LightningEntity(World* world, double x, double y, double z);

    void tick() override;

    long long seed = 0;

private:
    int ambientTick = 2;
    int remainingActions = 0;
};

} // namespace net::minecraft::entity
