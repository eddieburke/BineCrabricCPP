#pragma once
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity::model {
class CreeperEntityModel : public EntityModel {
 public:
 net::minecraft::client::model::ModelPart head{0, 0};
 net::minecraft::client::model::ModelPart hat{32, 0};
 net::minecraft::client::model::ModelPart body{16, 16};
 net::minecraft::client::model::ModelPart rightHindLeg{0, 16};
 net::minecraft::client::model::ModelPart leftHindLeg{0, 16};
 net::minecraft::client::model::ModelPart rightFrontLeg{0, 16};
 net::minecraft::client::model::ModelPart leftFrontLeg{0, 16};
 CreeperEntityModel() : CreeperEntityModel(0.0f) {
 }
 explicit CreeperEntityModel(float dilation) {
  const int n = 4;
  head = net::minecraft::client::model::ModelPart(0, 0);
  head.addCuboid(-4.0f, -8.0f, -4.0f, 8, 8, 8, dilation);
  head.setPivot(0.0f, static_cast<float>(n), 0.0f);
  hat = net::minecraft::client::model::ModelPart(32, 0);
  hat.addCuboid(-4.0f, -8.0f, -4.0f, 8, 8, 8, dilation + 0.5f);
  hat.setPivot(0.0f, static_cast<float>(n), 0.0f);
  body = net::minecraft::client::model::ModelPart(16, 16);
  body.addCuboid(-4.0f, 0.0f, -2.0f, 8, 12, 4, dilation);
  body.setPivot(0.0f, static_cast<float>(n), 0.0f);
  rightHindLeg = net::minecraft::client::model::ModelPart(0, 16);
  rightHindLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, dilation);
  rightHindLeg.setPivot(-2.0f, static_cast<float>(12 + n), 4.0f);
  leftHindLeg = net::minecraft::client::model::ModelPart(0, 16);
  leftHindLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, dilation);
  leftHindLeg.setPivot(2.0f, static_cast<float>(12 + n), 4.0f);
  rightFrontLeg = net::minecraft::client::model::ModelPart(0, 16);
  rightFrontLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, dilation);
  rightFrontLeg.setPivot(-2.0f, static_cast<float>(12 + n), -4.0f);
  leftFrontLeg = net::minecraft::client::model::ModelPart(0, 16);
  leftFrontLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, dilation);
  leftFrontLeg.setPivot(2.0f, static_cast<float>(12 + n), -4.0f);
 }
 void render(float limbAngle,
             float limbDistance,
             float animationProgress,
             float headYaw,
             float headPitch,
             float scale) override {
  setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
  head.render(scale);
  body.render(scale);
  rightHindLeg.render(scale);
  leftHindLeg.render(scale);
  rightFrontLeg.render(scale);
  leftFrontLeg.render(scale);
 }
 void setAngles(float limbAngle,
                float limbDistance,
                float animationProgress,
                float headYaw,
                float headPitch,
                float scale) override {
  (void)animationProgress;
  (void)scale;
  constexpr float kPi = 3.14159265358979323846f;
  head.yaw = headYaw / 57.295776f;
  head.pitch = headPitch / 57.295776f;
  rightHindLeg.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
  leftHindLeg.pitch =
      net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f + kPi) * 1.4f * limbDistance;
  rightFrontLeg.pitch =
      net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f + kPi) * 1.4f * limbDistance;
  leftFrontLeg.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
  applyPartOverrides();
 }
 void collectNamedParts(std::unordered_map<std::string, net::minecraft::client::model::ModelPart*>& parts) override {
  parts["head"] = &head;
  parts["hat"] = &hat;
  parts["body"] = &body;
  parts["rightHindLeg"] = &rightHindLeg;
  parts["leftHindLeg"] = &leftHindLeg;
  parts["rightFrontLeg"] = &rightFrontLeg;
  parts["leftFrontLeg"] = &leftFrontLeg;
 }
};
} // namespace net::minecraft::client::render::entity::model
