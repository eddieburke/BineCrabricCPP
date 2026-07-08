#pragma once
#include "net/minecraft/entity/LivingEntity.hpp"

namespace net::minecraft::entity::movement {
// Centralizes LivingEntity travel/jump motion (Java: LivingEntity.travel, tickMovement).
// Reference: mcp/src/net/minecraft/entity/LivingEntity.java
//            mcp/src/net/minecraft/entity/Entity.java (step-up / getEntitiesInside)
class PlayerMovement {
   public:
    enum class Medium {
        Water,
        Lava,
        Ground,
        Flying
    };
    static void travel(LivingEntity& entity, float sideways, float forward, float speedMultiplier = 1.0f);
    static void travelFlying(LivingEntity& entity, float sideways, float forward, float speedMultiplier = 1.0f);
    static void applyJumpInput(LivingEntity& entity);
    static void updateWalkAnimation(LivingEntity& entity);
    // Entity.move step-up gate (Java Entity.move n9).
    [[nodiscard]] static bool canStepUp(bool onGround, double intendedDy, double actualDy);
};
}  // namespace net::minecraft::entity::movement
