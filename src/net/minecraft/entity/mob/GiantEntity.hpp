#pragma once

#include "net/minecraft/entity/mob/MonsterEntity.hpp"

namespace net::minecraft::entity::mob {

class GiantEntity : public MonsterEntity {
public:
    explicit GiantEntity(World* world = nullptr);

protected:
    float getPathfindingFavor(int x, int y, int z) const override;
};

} // namespace net::minecraft::entity::mob
