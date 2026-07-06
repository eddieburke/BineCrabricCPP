#pragma once
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity::model {
class BipedEntityModel : public EntityModel {
public:
  net::minecraft::client::model::ModelPart head{0, 0};
  net::minecraft::client::model::ModelPart hat{32, 0};
  net::minecraft::client::model::ModelPart body{16, 16};
  net::minecraft::client::model::ModelPart rightArm{40, 16};
  net::minecraft::client::model::ModelPart leftArm{40, 16};
  net::minecraft::client::model::ModelPart rightLeg{0, 16};
  net::minecraft::client::model::ModelPart leftLeg{0, 16};
  net::minecraft::client::model::ModelPart ears{24, 0};
  net::minecraft::client::model::ModelPart cape{0, 0};
  bool leftArmPose = false;
  bool rightArmPose = false;
  bool sneaking = false;
  BipedEntityModel() : BipedEntityModel(0.0f) {}
  explicit BipedEntityModel(float dilation) : BipedEntityModel(dilation, 0.0f) {}
  explicit BipedEntityModel(float dilation, float pivotOffsetY) {
    cape.addCuboid(-5.0f, 0.0f, -1.0f, 10, 16, 1, dilation);
    ears = net::minecraft::client::model::ModelPart(24, 0);
    ears.addCuboid(-3.0f, -6.0f, -1.0f, 6, 6, 1, dilation);
    head = net::minecraft::client::model::ModelPart(0, 0);
    head.addCuboid(-4.0f, -8.0f, -4.0f, 8, 8, 8, dilation);
    head.setPivot(0.0f, pivotOffsetY, 0.0f);
    hat = net::minecraft::client::model::ModelPart(32, 0);
    hat.addCuboid(-4.0f, -8.0f, -4.0f, 8, 8, 8, dilation + 0.5f);
    hat.setPivot(0.0f, pivotOffsetY, 0.0f);
    body = net::minecraft::client::model::ModelPart(16, 16);
    body.addCuboid(-4.0f, 0.0f, -2.0f, 8, 12, 4, dilation);
    body.setPivot(0.0f, pivotOffsetY, 0.0f);
    rightArm = net::minecraft::client::model::ModelPart(40, 16);
    rightArm.addCuboid(-3.0f, -2.0f, -2.0f, 4, 12, 4, dilation);
    rightArm.setPivot(-5.0f, 2.0f + pivotOffsetY, 0.0f);
    leftArm = net::minecraft::client::model::ModelPart(40, 16);
    leftArm.mirror = true;
    leftArm.addCuboid(-1.0f, -2.0f, -2.0f, 4, 12, 4, dilation);
    leftArm.setPivot(5.0f, 2.0f + pivotOffsetY, 0.0f);
    rightLeg = net::minecraft::client::model::ModelPart(0, 16);
    rightLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 12, 4, dilation);
    rightLeg.setPivot(-2.0f, 12.0f + pivotOffsetY, 0.0f);
    leftLeg = net::minecraft::client::model::ModelPart(0, 16);
    leftLeg.mirror = true;
    leftLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 12, 4, dilation);
    leftLeg.setPivot(2.0f, 12.0f + pivotOffsetY, 0.0f);
  }
  void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch,
              float scale) override {
    setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
    head.render(scale);
    body.render(scale);
    rightArm.render(scale);
    leftArm.render(scale);
    rightLeg.render(scale);
    leftLeg.render(scale);
    hat.render(scale);
  }
  void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch,
                 float scale) override {
    (void)scale;
    constexpr float kPi = 3.14159265358979323846f;
    head.yaw = headYaw / 57.295776f;
    head.pitch = headPitch / 57.295776f;
    hat.yaw = head.yaw;
    hat.pitch = head.pitch;
    rightArm.pitch =
        net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f + kPi) * 2.0f * limbDistance * 0.5f;
    leftArm.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f) * 2.0f * limbDistance * 0.5f;
    rightArm.roll = 0.0f;
    leftArm.roll = 0.0f;
    rightLeg.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
    leftLeg.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f + kPi) * 1.4f * limbDistance;
    rightLeg.yaw = 0.0f;
    leftLeg.yaw = 0.0f;
    if(riding) {
      rightArm.pitch += -0.62831855f;
      leftArm.pitch += -0.62831855f;
      rightLeg.pitch = -1.2566371f;
      leftLeg.pitch = -1.2566371f;
      rightLeg.yaw = 0.31415927f;
      leftLeg.yaw = -0.31415927f;
    }
    if(leftArmPose) {
      leftArm.pitch = leftArm.pitch * 0.5f - 0.31415927f;
    }
    if(rightArmPose) {
      rightArm.pitch = rightArm.pitch * 0.5f - 0.31415927f;
    }
    rightArm.yaw = 0.0f;
    leftArm.yaw = 0.0f;
    if(handSwingProgress > -9990.0f) {
      float swingProgress = handSwingProgress;
      body.yaw = net::minecraft::util::math::MathHelper::sin(
                     net::minecraft::util::math::MathHelper::sqrt(swingProgress) * kPi * 2.0f) *
                 0.2f;
      rightArm.pivotZ = net::minecraft::util::math::MathHelper::sin(body.yaw) * 5.0f;
      rightArm.pivotX = -net::minecraft::util::math::MathHelper::cos(body.yaw) * 5.0f;
      leftArm.pivotZ = -net::minecraft::util::math::MathHelper::sin(body.yaw) * 5.0f;
      leftArm.pivotX = net::minecraft::util::math::MathHelper::cos(body.yaw) * 5.0f;
      rightArm.yaw += body.yaw;
      leftArm.yaw += body.yaw;
      leftArm.pitch += body.yaw;
      swingProgress = 1.0f - handSwingProgress;
      swingProgress *= swingProgress;
      swingProgress *= swingProgress;
      swingProgress = 1.0f - swingProgress;
      const float armSwingSin = net::minecraft::util::math::MathHelper::sin(swingProgress * kPi);
      const float armSwingAdjust =
          net::minecraft::util::math::MathHelper::sin(handSwingProgress * kPi) * -(head.pitch - 0.7f) * 0.75f;
      rightArm.pitch = rightArm.pitch - (armSwingSin * 1.2f + armSwingAdjust);
      rightArm.yaw += body.yaw * 2.0f;
      rightArm.roll = net::minecraft::util::math::MathHelper::sin(handSwingProgress * kPi) * -0.4f;
    }
    if(sneaking) {
      body.pitch = 0.5f;
      rightLeg.pitch -= 0.0f;
      leftLeg.pitch -= 0.0f;
      rightArm.pitch += 0.4f;
      leftArm.pitch += 0.4f;
      rightLeg.pivotZ = 4.0f;
      leftLeg.pivotZ = 4.0f;
      rightLeg.pivotY = 9.0f;
      leftLeg.pivotY = 9.0f;
      head.pivotY = 1.0f;
    } else {
      body.pitch = 0.0f;
      rightLeg.pivotZ = 0.0f;
      leftLeg.pivotZ = 0.0f;
      rightLeg.pivotY = 12.0f;
      leftLeg.pivotY = 12.0f;
      head.pivotY = 0.0f;
    }
    rightArm.roll += net::minecraft::util::math::MathHelper::cos(animationProgress * 0.09f) * 0.05f + 0.05f;
    leftArm.roll -= net::minecraft::util::math::MathHelper::cos(animationProgress * 0.09f) * 0.05f + 0.05f;
    rightArm.pitch += net::minecraft::util::math::MathHelper::sin(animationProgress * 0.067f) * 0.05f;
    leftArm.pitch -= net::minecraft::util::math::MathHelper::sin(animationProgress * 0.067f) * 0.05f;
  }
  void renderEars(float scale) {
    ears.yaw = head.yaw;
    ears.pitch = head.pitch;
    ears.pivotX = 0.0f;
    ears.pivotY = 0.0f;
    ears.render(scale);
  }
  void renderCape(float scale) {
    cape.render(scale);
  }
};
} // namespace net::minecraft::client::render::entity::model
