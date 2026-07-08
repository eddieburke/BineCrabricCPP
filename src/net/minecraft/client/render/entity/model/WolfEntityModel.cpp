#include "net/minecraft/client/render/entity/model/WolfEntityModel.hpp"

#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/entity/passive/WolfEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::client::render::entity::model {
void WolfEntityModel::render(
    float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
    setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
    head.renderForceTransform(scale);
    torso.render(scale);
    rightHindLeg.render(scale);
    leftHindLeg.render(scale);
    rightFrontLeg.render(scale);
    leftFrontLeg.render(scale);
    leftEar.renderForceTransform(scale);
    rightEar.renderForceTransform(scale);
    snout.renderForceTransform(scale);
    tail.renderForceTransform(scale);
    neck.render(scale);
}

void WolfEntityModel::animateModel(::net::minecraft::entity::LivingEntity& entity,
                                   float limbAngle,
                                   float limbDistance,
                                   float tickDelta) {
    auto* wolfEntity = dynamic_cast<net::minecraft::entity::passive::WolfEntity*>(&entity);
    if (wolfEntity == nullptr) {
        return;
    }
    constexpr float kPi = 3.14159265358979323846f;
    tail.yaw = wolfEntity->isAngry()
                   ? 0.0f
                   : net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
    if (wolfEntity->isInSittingPose()) {
        neck.setPivot(-1.0f, 16.0f, -3.0f);
        neck.pitch = 1.2566371f;
        neck.yaw = 0.0f;
        torso.setPivot(0.0f, 18.0f, 0.0f);
        torso.pitch = 0.7853982f;
        tail.setPivot(-1.0f, 21.0f, 6.0f);
        rightHindLeg.setPivot(-2.5f, 22.0f, 2.0f);
        rightHindLeg.pitch = 4.712389f;
        leftHindLeg.setPivot(0.5f, 22.0f, 2.0f);
        leftHindLeg.pitch = 4.712389f;
        rightFrontLeg.pitch = 5.811947f;
        rightFrontLeg.setPivot(-2.49f, 17.0f, -4.0f);
        leftFrontLeg.pitch = 5.811947f;
        leftFrontLeg.setPivot(0.51f, 17.0f, -4.0f);
    } else {
        torso.setPivot(0.0f, 14.0f, 2.0f);
        torso.pitch = 1.5707964f;
        neck.setPivot(-1.0f, 14.0f, -3.0f);
        neck.pitch = torso.pitch;
        tail.setPivot(-1.0f, 12.0f, 8.0f);
        rightHindLeg.setPivot(-2.5f, 16.0f, 7.0f);
        leftHindLeg.setPivot(0.5f, 16.0f, 7.0f);
        rightFrontLeg.setPivot(-2.5f, 16.0f, -4.0f);
        leftFrontLeg.setPivot(0.5f, 16.0f, -4.0f);
        rightHindLeg.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
        leftHindLeg.pitch =
            net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f + kPi) * 1.4f * limbDistance;
        rightFrontLeg.pitch =
            net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f + kPi) * 1.4f * limbDistance;
        leftFrontLeg.pitch = net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f) * 1.4f * limbDistance;
    }
    const float headRollAngle =
        wolfEntity->getBegAnimationProgress(tickDelta) + wolfEntity->getShakeAnimationProgress(tickDelta, 0.0f);
    head.roll = headRollAngle;
    leftEar.roll = headRollAngle;
    rightEar.roll = headRollAngle;
    snout.roll = headRollAngle;
    neck.roll = wolfEntity->getShakeAnimationProgress(tickDelta, -0.08f);
    torso.roll = wolfEntity->getShakeAnimationProgress(tickDelta, -0.16f);
    tail.roll = wolfEntity->getShakeAnimationProgress(tickDelta, -0.2f);
    if (wolfEntity->isFurWet()) {
        const float furBrightness =
            wolfEntity->getBrightnessAtEyes(tickDelta) * wolfEntity->getFurBrightnessMultiplier(tickDelta);
        net::minecraft::client::gl::color3f(furBrightness, furBrightness, furBrightness);
    }
}

void WolfEntityModel::setAngles(
    float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) {
    EntityModel::setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
    head.pitch = headPitch / 57.295776f;
    leftEar.yaw = head.yaw = headYaw / 57.295776f;
    leftEar.pitch = head.pitch;
    rightEar.yaw = head.yaw;
    rightEar.pitch = head.pitch;
    snout.yaw = head.yaw;
    snout.pitch = head.pitch;
    tail.pitch = animationProgress;
}
}  // namespace net::minecraft::client::render::entity::model
