#pragma once
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/QuadrupedEntityModel.hpp"

namespace net::minecraft::client::render::entity::model {
class CowEntityModel : public QuadrupedEntityModel {
   public:
    net::minecraft::client::model::ModelPart udder{52, 0};
    net::minecraft::client::model::ModelPart rightHorn{22, 0};
    net::minecraft::client::model::ModelPart leftHorn{22, 0};

    CowEntityModel() : QuadrupedEntityModel(12, 0.0f) {
        head = net::minecraft::client::model::ModelPart(0, 0);
        head.addCuboid(-4.0f, -4.0f, -6.0f, 8, 8, 6, 0.0f);
        head.setPivot(0.0f, 4.0f, -8.0f);
        rightHorn = net::minecraft::client::model::ModelPart(22, 0);
        rightHorn.addCuboid(-4.0f, -5.0f, -4.0f, 1, 3, 1, 0.0f);
        rightHorn.setPivot(0.0f, 3.0f, -7.0f);
        leftHorn = net::minecraft::client::model::ModelPart(22, 0);
        leftHorn.addCuboid(3.0f, -5.0f, -4.0f, 1, 3, 1, 0.0f);
        leftHorn.setPivot(0.0f, 3.0f, -7.0f);
        udder = net::minecraft::client::model::ModelPart(52, 0);
        udder.addCuboid(-2.0f, -3.0f, 0.0f, 4, 6, 2, 0.0f);
        udder.setPivot(0.0f, 14.0f, 6.0f);
        udder.pitch = 1.5707964f;
        body = net::minecraft::client::model::ModelPart(18, 4);
        body.addCuboid(-6.0f, -10.0f, -7.0f, 12, 18, 10, 0.0f);
        body.setPivot(0.0f, 5.0f, 2.0f);
        rightHindLeg.pivotX -= 1.0f;
        leftHindLeg.pivotX += 1.0f;
        rightFrontLeg.pivotX -= 1.0f;
        leftFrontLeg.pivotX += 1.0f;
        rightFrontLeg.pivotZ -= 1.0f;
        leftFrontLeg.pivotZ -= 1.0f;
        rightHorn.setPivot(0.0f, -1.0f, 1.0f);
        leftHorn.setPivot(0.0f, -1.0f, 1.0f);
        head.addChild(rightHorn);
        head.addChild(leftHorn);
    }

    void render(float limbAngle,
                float limbDistance,
                float animationProgress,
                float headYaw,
                float headPitch,
                float scale) override {
        QuadrupedEntityModel::render(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        udder.render(scale);
    }

    void setAngles(float limbAngle,
                   float limbDistance,
                   float animationProgress,
                   float headYaw,
                   float headPitch,
                   float scale) override {
        QuadrupedEntityModel::setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
    }
};
}  // namespace net::minecraft::client::render::entity::model
