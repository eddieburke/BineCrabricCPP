#pragma once
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::client::render::entity::model {
class SpiderEntityModel : public EntityModel {
   public:
    net::minecraft::client::model::ModelPart head{32, 4};
    net::minecraft::client::model::ModelPart body0{0, 0};
    net::minecraft::client::model::ModelPart body1{0, 12};
    net::minecraft::client::model::ModelPart rightHindLeg{18, 0};
    net::minecraft::client::model::ModelPart leftHindLeg{18, 0};
    net::minecraft::client::model::ModelPart rightMiddleHindLeg{18, 0};
    net::minecraft::client::model::ModelPart leftMiddleHindLeg{18, 0};
    net::minecraft::client::model::ModelPart rightMiddleFrontLeg{18, 0};
    net::minecraft::client::model::ModelPart leftMiddleFrontLeg{18, 0};
    net::minecraft::client::model::ModelPart rightFrontLeg{18, 0};
    net::minecraft::client::model::ModelPart leftFrontLeg{18, 0};

    SpiderEntityModel() {
        constexpr float inflation = 0.0f;
        constexpr int bodyPivotY = 15;
        head = net::minecraft::client::model::ModelPart(32, 4);
        head.addCuboid(-4.0f, -4.0f, -8.0f, 8, 8, 8, inflation);
        head.setPivot(0.0f, static_cast<float>(bodyPivotY), -3.0f);
        body0 = net::minecraft::client::model::ModelPart(0, 0);
        body0.addCuboid(-3.0f, -3.0f, -3.0f, 6, 6, 6, inflation);
        body0.setPivot(0.0f, static_cast<float>(bodyPivotY), 0.0f);
        body1 = net::minecraft::client::model::ModelPart(0, 12);
        body1.addCuboid(-5.0f, -4.0f, -6.0f, 10, 8, 12, inflation);
        body1.setPivot(0.0f, static_cast<float>(bodyPivotY), 9.0f);
        rightHindLeg = net::minecraft::client::model::ModelPart(18, 0);
        rightHindLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, inflation);
        rightHindLeg.setPivot(-4.0f, static_cast<float>(bodyPivotY), 2.0f);
        leftHindLeg = net::minecraft::client::model::ModelPart(18, 0);
        leftHindLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, inflation);
        leftHindLeg.setPivot(4.0f, static_cast<float>(bodyPivotY), 2.0f);
        rightMiddleHindLeg = net::minecraft::client::model::ModelPart(18, 0);
        rightMiddleHindLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, inflation);
        rightMiddleHindLeg.setPivot(-4.0f, static_cast<float>(bodyPivotY), 1.0f);
        leftMiddleHindLeg = net::minecraft::client::model::ModelPart(18, 0);
        leftMiddleHindLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, inflation);
        leftMiddleHindLeg.setPivot(4.0f, static_cast<float>(bodyPivotY), 1.0f);
        rightMiddleFrontLeg = net::minecraft::client::model::ModelPart(18, 0);
        rightMiddleFrontLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, inflation);
        rightMiddleFrontLeg.setPivot(-4.0f, static_cast<float>(bodyPivotY), 0.0f);
        leftMiddleFrontLeg = net::minecraft::client::model::ModelPart(18, 0);
        leftMiddleFrontLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, inflation);
        leftMiddleFrontLeg.setPivot(4.0f, static_cast<float>(bodyPivotY), 0.0f);
        rightFrontLeg = net::minecraft::client::model::ModelPart(18, 0);
        rightFrontLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, inflation);
        rightFrontLeg.setPivot(-4.0f, static_cast<float>(bodyPivotY), -1.0f);
        leftFrontLeg = net::minecraft::client::model::ModelPart(18, 0);
        leftFrontLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, inflation);
        leftFrontLeg.setPivot(4.0f, static_cast<float>(bodyPivotY), -1.0f);
    }

    void render(float limbAngle,
                float limbDistance,
                float animationProgress,
                float headYaw,
                float headPitch,
                float scale) override {
        setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        head.render(scale);
        body0.render(scale);
        body1.render(scale);
        rightHindLeg.render(scale);
        leftHindLeg.render(scale);
        rightMiddleHindLeg.render(scale);
        leftMiddleHindLeg.render(scale);
        rightMiddleFrontLeg.render(scale);
        leftMiddleFrontLeg.render(scale);
        rightFrontLeg.render(scale);
        leftFrontLeg.render(scale);
    }

    void setAngles(float limbAngle,
                   float limbDistance,
                   float animationProgress,
                   float headYaw,
                   float headPitch,
                   float scale) override {
        (void) animationProgress;
        (void) scale;
        constexpr float kPi = 3.14159265358979323846f;
        head.yaw = headYaw / 57.295776f;
        head.pitch = headPitch / 57.295776f;
        const float legRollAngle = 0.7853982f;
        rightHindLeg.roll = -legRollAngle;
        leftHindLeg.roll = legRollAngle;
        rightMiddleHindLeg.roll = -legRollAngle * 0.74f;
        leftMiddleHindLeg.roll = legRollAngle * 0.74f;
        rightMiddleFrontLeg.roll = -legRollAngle * 0.74f;
        leftMiddleFrontLeg.roll = legRollAngle * 0.74f;
        rightFrontLeg.roll = -legRollAngle;
        leftFrontLeg.roll = legRollAngle;
        const float legYawOffset = 0.0f;
        const float legYawSpread = 0.3926991f;
        rightHindLeg.yaw = legYawSpread * 2.0f + legYawOffset;
        leftHindLeg.yaw = -legYawSpread * 2.0f - legYawOffset;
        rightMiddleHindLeg.yaw = legYawSpread * 1.0f + legYawOffset;
        leftMiddleHindLeg.yaw = -legYawSpread * 1.0f - legYawOffset;
        rightMiddleFrontLeg.yaw = -legYawSpread * 1.0f + legYawOffset;
        leftMiddleFrontLeg.yaw = legYawSpread * 1.0f - legYawOffset;
        rightFrontLeg.yaw = -legYawSpread * 2.0f + legYawOffset;
        leftFrontLeg.yaw = legYawSpread * 2.0f - legYawOffset;
        const float hindLegYawSwing =
            -(net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f * 2.0f + 0.0f) * 0.4f) * limbDistance;
        const float middleHindLegYawSwing =
            -(net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f * 2.0f + kPi) * 0.4f) * limbDistance;
        const float middleFrontLegYawSwing =
            -(net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f * 2.0f + 1.5707964f) * 0.4f) *
            limbDistance;
        const float frontLegYawSwing =
            -(net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f * 2.0f + 4.712389f) * 0.4f) *
            limbDistance;
        const float hindLegRollSwing =
            net::minecraft::util::math::MathHelper::abs(
                net::minecraft::util::math::MathHelper::sin(limbAngle * 0.6662f + 0.0f) * 0.4f) *
            limbDistance;
        const float middleHindLegRollSwing =
            net::minecraft::util::math::MathHelper::abs(
                net::minecraft::util::math::MathHelper::sin(limbAngle * 0.6662f + kPi) * 0.4f) *
            limbDistance;
        const float middleFrontLegRollSwing =
            net::minecraft::util::math::MathHelper::abs(
                net::minecraft::util::math::MathHelper::sin(limbAngle * 0.6662f + 1.5707964f) * 0.4f) *
            limbDistance;
        const float frontLegRollSwing =
            net::minecraft::util::math::MathHelper::abs(
                net::minecraft::util::math::MathHelper::sin(limbAngle * 0.6662f + 4.712389f) * 0.4f) *
            limbDistance;
        rightHindLeg.yaw += hindLegYawSwing;
        leftHindLeg.yaw += -hindLegYawSwing;
        rightMiddleHindLeg.yaw += middleHindLegYawSwing;
        leftMiddleHindLeg.yaw += -middleHindLegYawSwing;
        rightMiddleFrontLeg.yaw += middleFrontLegYawSwing;
        leftMiddleFrontLeg.yaw += -middleFrontLegYawSwing;
        rightFrontLeg.yaw += frontLegYawSwing;
        leftFrontLeg.yaw += -frontLegYawSwing;
        rightHindLeg.roll += hindLegRollSwing;
        leftHindLeg.roll += -hindLegRollSwing;
        rightMiddleHindLeg.roll += middleHindLegRollSwing;
        leftMiddleHindLeg.roll += -middleHindLegRollSwing;
        rightMiddleFrontLeg.roll += middleFrontLegRollSwing;
        leftMiddleFrontLeg.roll += -middleFrontLegRollSwing;
        rightFrontLeg.roll += frontLegRollSwing;
        leftFrontLeg.roll += -frontLegRollSwing;
        applyPartOverrides();
    }

    void collectNamedParts(std::unordered_map<std::string, net::minecraft::client::model::ModelPart*>& parts) override {
        parts["head"] = &head;
        parts["body0"] = &body0;
        parts["body1"] = &body1;
        parts["rightHindLeg"] = &rightHindLeg;
        parts["leftHindLeg"] = &leftHindLeg;
        parts["rightMiddleHindLeg"] = &rightMiddleHindLeg;
        parts["leftMiddleHindLeg"] = &leftMiddleHindLeg;
        parts["rightMiddleFrontLeg"] = &rightMiddleFrontLeg;
        parts["leftMiddleFrontLeg"] = &leftMiddleFrontLeg;
        parts["rightFrontLeg"] = &rightFrontLeg;
        parts["leftFrontLeg"] = &leftFrontLeg;
    }
};
}  // namespace net::minecraft::client::render::entity::model
