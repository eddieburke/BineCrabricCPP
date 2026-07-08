#pragma once
#include <unordered_set>
#include <vector>

#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft::entity {
class Entity;
}

namespace net::minecraft {
class World;

class Explosion {
   public:
    Explosion(World* world, entity::Entity* source, double x, double y, double z, float power);
    void explode();
    void playExplosionSound(bool addParticles);
    bool fire = false;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    entity::Entity* source = nullptr;
    float power = 0.0f;
    std::unordered_set<Vec3i, Vec3iHash> damagedBlocks;

   private:
    JavaRandom random_;
    World* world_ = nullptr;
};
}  // namespace net::minecraft
