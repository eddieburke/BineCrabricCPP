#pragma once
#include "net/minecraft/client/render/entity/model/BipedEntityModel.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::client::render::entity::model {
class ZombieEntityModel : public BipedEntityModel {
   public:
    using BipedEntityModel::BipedEntityModel;

    void setAngles(float limbAngle,
                   float limbDistance,
                   float animationProgress,
                   float headYaw,
                   float headPitch,
                   float scale) override {
        BipedEntityModel::setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        constexpr float kPi = 3.14159265358979323846f;
        const float swingSin = net::minecraft::util::math::MathHelper::sin(handSwingProgress * kPi);
        const float swingEase = net::minecraft::util::math::MathHelper::sin(
            (1.0f - (1.0f - handSwingProgress) * (1.0f - handSwingProgress)) * kPi);
        rightArm.roll = 0.0f;
        leftArm.roll = 0.0f;
        rightArm.yaw = -(0.1f - swingSin * 0.6f);
        leftArm.yaw = 0.1f - swingSin * 0.6f;
        rightArm.pitch = -1.5707964f;
        leftArm.pitch = -1.5707964f;
        rightArm.pitch -= swingSin * 1.2f - swingEase * 0.4f;
        leftArm.pitch -= swingSin * 1.2f - swingEase * 0.4f;
        rightArm.roll += net::minecraft::util::math::MathHelper::cos(animationProgress * 0.09f) * 0.05f + 0.05f;
        leftArm.roll -= net::minecraft::util::math::MathHelper::cos(animationProgress * 0.09f) * 0.05f + 0.05f;
        rightArm.pitch += net::minecraft::util::math::MathHelper::sin(animationProgress * 0.067f) * 0.05f;
        leftArm.pitch -= net::minecraft::util::math::MathHelper::sin(animationProgress * 0.067f) * 0.05f;
    }
};
}  // namespace net::minecraft::client::render::entity::model
