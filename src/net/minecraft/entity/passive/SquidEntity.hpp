#pragma once
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/entity/WaterCreatureEntity.hpp"
namespace net::minecraft::entity::passive {
class SquidEntity : public WaterCreatureEntity {
 public:
 static constexpr int kEntityId = 94;
 static constexpr const char* kEntityName = "Squid";
 struct ClientRenderer {
  static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
 };
 explicit SquidEntity(World* world = nullptr);
 float lastTiltAngle = 0.0f;
 float tiltAngle = 0.0f;
 float lastRollAngle = 0.0f;
 float rollAngle = 0.0f;
 float lastTentacleAngle = 0.0f;
 float tentacleAngle = 0.0f;
 void tickMovement() override;
 void travel(float sideways, float forward) override;
 [[nodiscard]] bool isSubmergedInWater() const override;
 [[nodiscard]] std::string getRandomSound() override {
  return {};
 }
 [[nodiscard]] std::string getHurtSound() const override {
  return {};
 }
 [[nodiscard]] std::string getDeathSound() const override {
  return {};
 }
 [[nodiscard]] float getSoundVolume() const override {
  return 0.4f;
 }
 void dropItems() override;

 protected:
 void tickLiving() override;

 private:
 float thrustTimer = 0.0f;
 float lastThrustTimer = 0.0f;
 float swimVelocityScale = 0.0f;
 float thrustTimerSpeed = 0.0f;
 float turningSpeed = 0.0f;
 float swimX = 0.0f;
 float swimY = 0.0f;
 float swimZ = 0.0f;
};
} // namespace net::minecraft::entity::passive
