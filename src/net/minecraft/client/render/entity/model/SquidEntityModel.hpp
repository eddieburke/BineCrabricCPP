#pragma once

#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

#include <array>
#include <cmath>

namespace net::minecraft::client::render::entity::model {

class SquidEntityModel : public EntityModel {
public:
    net::minecraft::client::model::ModelPart root;
    std::array<net::minecraft::client::model::ModelPart, 8> tentacles;

    SquidEntityModel()
    {
        const int n = -16;
        root = net::minecraft::client::model::ModelPart(0, 0);
        root.addCuboid(-6.0f, -8.0f, -6.0f, 12, 16, 12);
        root.pivotY += static_cast<float>(24 + n);

        for (std::size_t i = 0; i < tentacles.size(); ++i) {
            tentacles[i] = net::minecraft::client::model::ModelPart(48, 0);
            double d = static_cast<double>(i) * 3.14159265358979323846 * 2.0 / static_cast<double>(tentacles.size());
            const float f = static_cast<float>(std::cos(d) * 5.0);
            const float f2 = static_cast<float>(std::sin(d) * 5.0);
            tentacles[i].addCuboid(-1.0f, 0.0f, -1.0f, 2, 18, 2);
            tentacles[i].pivotX = f;
            tentacles[i].pivotZ = f2;
            tentacles[i].pivotY = static_cast<float>(31 + n);
            d = static_cast<double>(i) * 3.14159265358979323846 * -2.0 / static_cast<double>(tentacles.size()) + 1.5707963267948966;
            tentacles[i].yaw = static_cast<float>(d);
        }
    }

    void setAngles(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) override
    {
        (void)limbAngle;
        (void)limbDistance;
        (void)headYaw;
        (void)headPitch;
        (void)scale;
        for (auto& tentacle : tentacles) {
            tentacle.pitch = animationProgress;
        }
    }

    void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) override
    {
        setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
        root.render(scale);
        for (auto& tentacle : tentacles) {
            tentacle.render(scale);
        }
    }
};

} // namespace net::minecraft::client::render::entity::model
