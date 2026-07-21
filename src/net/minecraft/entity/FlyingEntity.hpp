#pragma once
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/movement/PlayerMovement.hpp"
namespace net::minecraft::entity {
class FlyingEntity : public LivingEntity {
 public:
 explicit FlyingEntity(World* world = nullptr) : LivingEntity(world) {
 }
 void onLanding(float /*fallDistance*/) override {
 }
 void travel(float sideways, float forward) override {
  movement::PlayerMovement::travelFlying(*this, sideways, forward);
 }
 [[nodiscard]] bool isOnLadder() const override {
  return false;
 }
};
} // namespace net::minecraft::entity
