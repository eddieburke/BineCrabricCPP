#pragma once
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity::model {
class ChickenEntityModel : public EntityModel {
 public:
 net::minecraft::client::model::ModelPart head{0, 0};
 net::minecraft::client::model::ModelPart body{0, 9};
 net::minecraft::client::model::ModelPart rightLeg{26, 0};
 net::minecraft::client::model::ModelPart leftLeg{26, 0};
 net::minecraft::client::model::ModelPart rightWing{24, 13};
 net::minecraft::client::model::ModelPart leftWing{24, 13};
 net::minecraft::client::model::ModelPart beak{14, 0};
 net::minecraft::client::model::ModelPart wattle{14, 4};
 ChickenEntityModel() {
  const int n = 16;
  head = net::minecraft::client::model::ModelPart(0, 0);
  head.addCuboid(-2.0f, -6.0f, -2.0f, 4, 6, 3, 0.0f);
  head.setPivot(0.0f, static_cast<float>(-1 + n), -4.0f);
  beak = net::minecraft::client::model::ModelPart(14, 0);
  beak.addCuboid(-2.0f, -4.0f, -4.0f, 4, 2, 2, 0.0f);
  beak.setPivot(0.0f, static_cast<float>(-1 + n), -4.0f);
  wattle = net::minecraft::client::model::ModelPart(14, 4);
  wattle.addCuboid(-1.0f, -2.0f, -3.0f, 2, 2, 2, 0.0f);
  wattle.setPivot(0.0f, static_cast<float>(-1 + n), -4.0f);
  body = net::minecraft::client::model::ModelPart(0, 9);
  body.addCuboid(-3.0f, -4.0f, -3.0f, 6, 8, 6, 0.0f);
  body.setPivot(0.0f, static_cast<float>(n), 0.0f);
  rightLeg = net::minecraft::client::model::ModelPart(26, 0);
  rightLeg.addCuboid(-1.0f, 0.0f, -3.0f, 3, 5, 3);
  rightLeg.setPivot(-2.0f, static_cast<float>(3 + n), 1.0f);
  leftLeg = net::minecraft::client::model::ModelPart(26, 0);
  leftLeg.addCuboid(-1.0f, 0.0f, -3.0f, 3, 5, 3);
  leftLeg.setPivot(1.0f, static_cast<float>(3 + n), 1.0f);
  rightWing = net::minecraft::client::model::ModelPart(24, 13);
  rightWing.addCuboid(0.0f, 0.0f, -3.0f, 1, 4, 6);
  rightWing.setPivot(-4.0f, static_cast<float>(-3 + n), 0.0f);
  leftWing = net::minecraft::client::model::ModelPart(24, 13);
  leftWing.addCuboid(-1.0f, 0.0f, -3.0f, 1, 4, 6);
  leftWing.setPivot(4.0f, static_cast<float>(-3 + n), 0.0f);
 }
 void render(float limbAngle,
             float limbDistance,
             float animationProgress,
             float headYaw,
             float headPitch,
             float scale) override {
  setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
  head.render(scale);
  beak.render(scale);
  wattle.render(scale);
  body.render(scale);
  rightLeg.render(scale);
  leftLeg.render(scale);
  rightWing.render(scale);
  leftWing.render(scale);
 }
 void setAngles(float limbAngle,
                float limbDistance,
                float animationProgress,
                float headYaw,
                float headPitch,
                float scale) override {
  (void)scale;
  constexpr float kPi = 3.14159265358979323846f;
  head.pitch = -(headPitch / 57.295776f);
  head.yaw = headYaw / 57.295776f;
  beak.pitch = head.pitch;
  beak.yaw = head.yaw;
  wattle.pitch = head.pitch;
  wattle.yaw = head.yaw;
  body.pitch = 1.5707964f;
  rightLeg.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
  leftLeg.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f + kPi) * 1.4f * limbDistance;
  rightWing.roll = animationProgress;
  leftWing.roll = -animationProgress;
 }
};
} // namespace net::minecraft::client::render::entity::model
