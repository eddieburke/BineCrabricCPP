#pragma once

#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::client::render::entity::model {

class SpiderEntityModel : public EntityModel {
public:
    net::minecraft::client::model::ModelPart head {32, 4};
    net::minecraft::client::model::ModelPart body0 {0, 0};
    net::minecraft::client::model::ModelPart body1 {0, 12};
    net::minecraft::client::model::ModelPart rightHindLeg {18, 0};
    net::minecraft::client::model::ModelPart leftHindLeg {18, 0};
    net::minecraft::client::model::ModelPart rightMiddleHindLeg {18, 0};
    net::minecraft::client::model::ModelPart leftMiddleHindLeg {18, 0};
    net::minecraft::client::model::ModelPart rightMiddleFrontLeg {18, 0};
    net::minecraft::client::model::ModelPart leftMiddleFrontLeg {18, 0};
    net::minecraft::client::model::ModelPart rightFrontLeg {18, 0};
    net::minecraft::client::model::ModelPart leftFrontLeg {18, 0};

    SpiderEntityModel()
    {
        const float f = 0.0f;
        const int n = 15;
        head = net::minecraft::client::model::ModelPart(32, 4);
        head.addCuboid(-4.0f, -4.0f, -8.0f, 8, 8, 8, f);
        head.setPivot(0.0f, static_cast<float>(n), -3.0f);

        body0 = net::minecraft::client::model::ModelPart(0, 0);
        body0.addCuboid(-3.0f, -3.0f, -3.0f, 6, 6, 6, f);
        body0.setPivot(0.0f, static_cast<float>(n), 0.0f);

        body1 = net::minecraft::client::model::ModelPart(0, 12);
        body1.addCuboid(-5.0f, -4.0f, -6.0f, 10, 8, 12, f);
        body1.setPivot(0.0f, static_cast<float>(n), 9.0f);

        rightHindLeg = net::minecraft::client::model::ModelPart(18, 0);
        rightHindLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, f);
        rightHindLeg.setPivot(-4.0f, static_cast<float>(n), 2.0f);

        leftHindLeg = net::minecraft::client::model::ModelPart(18, 0);
        leftHindLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, f);
        leftHindLeg.setPivot(4.0f, static_cast<float>(n), 2.0f);

        rightMiddleHindLeg = net::minecraft::client::model::ModelPart(18, 0);
        rightMiddleHindLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, f);
        rightMiddleHindLeg.setPivot(-4.0f, static_cast<float>(n), 1.0f);

        leftMiddleHindLeg = net::minecraft::client::model::ModelPart(18, 0);
        leftMiddleHindLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, f);
        leftMiddleHindLeg.setPivot(4.0f, static_cast<float>(n), 1.0f);

        rightMiddleFrontLeg = net::minecraft::client::model::ModelPart(18, 0);
        rightMiddleFrontLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, f);
        rightMiddleFrontLeg.setPivot(-4.0f, static_cast<float>(n), 0.0f);

        leftMiddleFrontLeg = net::minecraft::client::model::ModelPart(18, 0);
        leftMiddleFrontLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, f);
        leftMiddleFrontLeg.setPivot(4.0f, static_cast<float>(n), 0.0f);

        rightFrontLeg = net::minecraft::client::model::ModelPart(18, 0);
        rightFrontLeg.addCuboid(-15.0f, -1.0f, -1.0f, 16, 2, 2, f);
        rightFrontLeg.setPivot(-4.0f, static_cast<float>(n), -1.0f);

        leftFrontLeg = net::minecraft::client::model::ModelPart(18, 0);
        leftFrontLeg.addCuboid(-1.0f, -1.0f, -1.0f, 16, 2, 2, f);
        leftFrontLeg.setPivot(4.0f, static_cast<float>(n), -1.0f);
    }

    void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) override
    {
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

    void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) override
    {
        (void)animationProgress;
        (void)scale;
        constexpr float kPi = 3.14159265358979323846f;
        head.yaw = headYaw / 57.295776f;
        head.pitch = headPitch / 57.295776f;
        const float f = 0.7853982f;
        rightHindLeg.roll = -f;
        leftHindLeg.roll = f;
        rightMiddleHindLeg.roll = -f * 0.74f;
        leftMiddleHindLeg.roll = f * 0.74f;
        rightMiddleFrontLeg.roll = -f * 0.74f;
        leftMiddleFrontLeg.roll = f * 0.74f;
        rightFrontLeg.roll = -f;
        leftFrontLeg.roll = f;
        const float f2 = -0.0f;
        const float f3 = 0.3926991f;
        rightHindLeg.yaw = f3 * 2.0f + f2;
        leftHindLeg.yaw = -f3 * 2.0f - f2;
        rightMiddleHindLeg.yaw = f3 * 1.0f + f2;
        leftMiddleHindLeg.yaw = -f3 * 1.0f - f2;
        rightMiddleFrontLeg.yaw = -f3 * 1.0f + f2;
        leftMiddleFrontLeg.yaw = f3 * 1.0f - f2;
        rightFrontLeg.yaw = -f3 * 2.0f + f2;
        leftFrontLeg.yaw = f3 * 2.0f - f2;
        const float f4 = -(net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f * 2.0f + 0.0f) * 0.4f) * limbDistance;
        const float f5 = -(net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f * 2.0f + kPi) * 0.4f) * limbDistance;
        const float f6 = -(net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f * 2.0f + 1.5707964f) * 0.4f) * limbDistance;
        const float f7 = -(net::minecraft::util::math::MathHelper::cos(limbAngle * 0.6662f * 2.0f + 4.712389f) * 0.4f) * limbDistance;
        const float f8 = net::minecraft::util::math::MathHelper::abs(net::minecraft::util::math::MathHelper::sin(limbAngle * 0.6662f + 0.0f) * 0.4f) * limbDistance;
        const float f9 = net::minecraft::util::math::MathHelper::abs(net::minecraft::util::math::MathHelper::sin(limbAngle * 0.6662f + kPi) * 0.4f) * limbDistance;
        const float f10 = net::minecraft::util::math::MathHelper::abs(net::minecraft::util::math::MathHelper::sin(limbAngle * 0.6662f + 1.5707964f) * 0.4f) * limbDistance;
        const float f11 = net::minecraft::util::math::MathHelper::abs(net::minecraft::util::math::MathHelper::sin(limbAngle * 0.6662f + 4.712389f) * 0.4f) * limbDistance;
        rightHindLeg.yaw += f4;
        leftHindLeg.yaw += -f4;
        rightMiddleHindLeg.yaw += f5;
        leftMiddleHindLeg.yaw += -f5;
        rightMiddleFrontLeg.yaw += f6;
        leftMiddleFrontLeg.yaw += -f6;
        rightFrontLeg.yaw += f7;
        leftFrontLeg.yaw += -f7;
        rightHindLeg.roll += f8;
        leftHindLeg.roll += -f8;
        rightMiddleHindLeg.roll += f9;
        leftMiddleHindLeg.roll += -f9;
        rightMiddleFrontLeg.roll += f10;
        leftMiddleFrontLeg.roll += -f10;
        rightFrontLeg.roll += f11;
        leftFrontLeg.roll += -f11;
    }
};

} // namespace net::minecraft::client::render::entity::model
