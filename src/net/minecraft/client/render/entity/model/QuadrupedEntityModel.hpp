#pragma once
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity::model {
class QuadrupedEntityModel : public EntityModel {
public:
  net::minecraft::client::model::ModelPart head{0, 0};
  net::minecraft::client::model::ModelPart body{28, 8};
  net::minecraft::client::model::ModelPart rightHindLeg{0, 16};
  net::minecraft::client::model::ModelPart leftHindLeg{0, 16};
  net::minecraft::client::model::ModelPart rightFrontLeg{0, 16};
  net::minecraft::client::model::ModelPart leftFrontLeg{0, 16};
  explicit QuadrupedEntityModel(int stanceWidth, float dilation) {
    head.addCuboid(-4.0f, -4.0f, -8.0f, 8, 8, 8, dilation);
    head.setPivot(0.0f, 18.0f - static_cast<float>(stanceWidth), -6.0f);
    body = net::minecraft::client::model::ModelPart(28, 8);
    body.addCuboid(-5.0f, -10.0f, -7.0f, 10, 16, 8, dilation);
    body.setPivot(0.0f, 17.0f - static_cast<float>(stanceWidth), 2.0f);
    rightHindLeg = net::minecraft::client::model::ModelPart(0, 16);
    rightHindLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, stanceWidth, 4, dilation);
    rightHindLeg.setPivot(-3.0f, 24.0f - static_cast<float>(stanceWidth), 7.0f);
    leftHindLeg = net::minecraft::client::model::ModelPart(0, 16);
    leftHindLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, stanceWidth, 4, dilation);
    leftHindLeg.setPivot(3.0f, 24.0f - static_cast<float>(stanceWidth), 7.0f);
    rightFrontLeg = net::minecraft::client::model::ModelPart(0, 16);
    rightFrontLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, stanceWidth, 4, dilation);
    rightFrontLeg.setPivot(-3.0f, 24.0f - static_cast<float>(stanceWidth), -5.0f);
    leftFrontLeg = net::minecraft::client::model::ModelPart(0, 16);
    leftFrontLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, stanceWidth, 4, dilation);
    leftFrontLeg.setPivot(3.0f, 24.0f - static_cast<float>(stanceWidth), -5.0f);
  }
  void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch,
              float scale) override {
    setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
    head.render(scale);
    body.render(scale);
    rightHindLeg.render(scale);
    leftHindLeg.render(scale);
    rightFrontLeg.render(scale);
    leftFrontLeg.render(scale);
  }
  void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch,
                 float scale) override {
    (void)animationProgress;
    (void)scale;
    constexpr float kPi = 3.14159265358979323846f;
    head.pitch = headPitch / 57.295776f;
    head.yaw = headYaw / 57.295776f;
    body.pitch = 1.5707964f;
    rightHindLeg.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
    leftHindLeg.pitch =
        net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f + kPi) * 1.4f * limbDistance;
    rightFrontLeg.pitch =
        net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f + kPi) * 1.4f * limbDistance;
    leftFrontLeg.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
  }
};
} // namespace net::minecraft::client::render::entity::model
