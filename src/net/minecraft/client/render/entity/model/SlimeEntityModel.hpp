#pragma once

#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

#include <optional>

namespace net::minecraft::client::render::entity::model {

class SlimeEntityModel : public EntityModel {
public:
    net::minecraft::client::model::ModelPart cube {0, 0};
    std::optional<net::minecraft::client::model::ModelPart> rightEye;
    std::optional<net::minecraft::client::model::ModelPart> leftEye;
    std::optional<net::minecraft::client::model::ModelPart> mouth;

    explicit SlimeEntityModel(int v = 0)
    {
        cube = net::minecraft::client::model::ModelPart(0, v);
        cube.addCuboid(-4.0f, 16.0f, -4.0f, 8, 8, 8);
        if (v > 0) {
            cube = net::minecraft::client::model::ModelPart(0, v);
            cube.addCuboid(-3.0f, 17.0f, -3.0f, 6, 6, 6);
            rightEye = net::minecraft::client::model::ModelPart(32, 0);
            rightEye->addCuboid(-3.25f, 18.0f, -3.5f, 2, 2, 2);
            leftEye = net::minecraft::client::model::ModelPart(32, 4);
            leftEye->addCuboid(1.25f, 18.0f, -3.5f, 2, 2, 2);
            mouth = net::minecraft::client::model::ModelPart(32, 8);
            mouth->addCuboid(0.0f, 21.0f, -3.5f, 1, 1, 1);
        }
    }

    void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) override
    {
        (void)limbAngle;
        (void)limbDistance;
        (void)animationProgress;
        (void)headYaw;
        (void)headPitch;
        (void)scale;
    }

    void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) override
    {
        setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        cube.render(scale);
        if (rightEye.has_value()) {
            rightEye->render(scale);
            leftEye->render(scale);
            mouth->render(scale);
        }
    }
};

} // namespace net::minecraft::client::render::entity::model
