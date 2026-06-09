#pragma once

#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"

namespace net::minecraft::client::render::entity::model {

class WolfEntityModel : public EntityModel {
public:
    net::minecraft::client::model::ModelPart head {0, 0};
    net::minecraft::client::model::ModelPart torso {18, 14};
    net::minecraft::client::model::ModelPart rightHindLeg {0, 18};
    net::minecraft::client::model::ModelPart leftHindLeg {0, 18};
    net::minecraft::client::model::ModelPart rightFrontLeg {0, 18};
    net::minecraft::client::model::ModelPart leftFrontLeg {0, 18};
    net::minecraft::client::model::ModelPart leftEar {16, 14};
    net::minecraft::client::model::ModelPart rightEar {16, 14};
    net::minecraft::client::model::ModelPart snout {0, 10};
    net::minecraft::client::model::ModelPart tail {9, 18};
    net::minecraft::client::model::ModelPart neck {21, 0};

    WolfEntityModel()
    {
        constexpr float inflation = 0.0f;
        constexpr float headPivotY = 13.5f;

        head = net::minecraft::client::model::ModelPart(0, 0);
        head.addCuboid(-3.0f, -3.0f, -2.0f, 6, 6, 4, inflation);
        head.setPivot(-1.0f, headPivotY, -7.0f);

        torso = net::minecraft::client::model::ModelPart(18, 14);
        torso.addCuboid(-4.0f, -2.0f, -3.0f, 6, 9, 6, inflation);
        torso.setPivot(0.0f, 14.0f, 2.0f);

        neck = net::minecraft::client::model::ModelPart(21, 0);
        neck.addCuboid(-4.0f, -3.0f, -3.0f, 8, 6, 7, inflation);
        neck.setPivot(-1.0f, 14.0f, 2.0f);

        rightHindLeg = net::minecraft::client::model::ModelPart(0, 18);
        rightHindLeg.addCuboid(-1.0f, 0.0f, -1.0f, 2, 8, 2, inflation);
        rightHindLeg.setPivot(-2.5f, 16.0f, 7.0f);

        leftHindLeg = net::minecraft::client::model::ModelPart(0, 18);
        leftHindLeg.addCuboid(-1.0f, 0.0f, -1.0f, 2, 8, 2, inflation);
        leftHindLeg.setPivot(0.5f, 16.0f, 7.0f);

        rightFrontLeg = net::minecraft::client::model::ModelPart(0, 18);
        rightFrontLeg.addCuboid(-1.0f, 0.0f, -1.0f, 2, 8, 2, inflation);
        rightFrontLeg.setPivot(-2.5f, 16.0f, -4.0f);

        leftFrontLeg = net::minecraft::client::model::ModelPart(0, 18);
        leftFrontLeg.addCuboid(-1.0f, 0.0f, -1.0f, 2, 8, 2, inflation);
        leftFrontLeg.setPivot(0.5f, 16.0f, -4.0f);

        tail = net::minecraft::client::model::ModelPart(9, 18);
        tail.addCuboid(-1.0f, 0.0f, -1.0f, 2, 8, 2, inflation);
        tail.setPivot(-1.0f, 12.0f, 8.0f);

        leftEar = net::minecraft::client::model::ModelPart(16, 14);
        leftEar.addCuboid(-3.0f, -5.0f, 0.0f, 2, 2, 1, inflation);
        leftEar.setPivot(-1.0f, headPivotY, -7.0f);

        rightEar = net::minecraft::client::model::ModelPart(16, 14);
        rightEar.addCuboid(1.0f, -5.0f, 0.0f, 2, 2, 1, inflation);
        rightEar.setPivot(-1.0f, headPivotY, -7.0f);

        snout = net::minecraft::client::model::ModelPart(0, 10);
        snout.addCuboid(-2.0f, 0.0f, -5.0f, 3, 3, 4, inflation);
        snout.setPivot(-0.5f, headPivotY, -7.0f);
    }

    void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) override;
    void animateModel(::net::minecraft::entity::LivingEntity& entity, float limbAngle, float limbDistance, float tickDelta) override;
    void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) override;
};

} // namespace net::minecraft::client::render::entity::model
