#pragma once

#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

#include <array>
#include <cmath>

namespace net::minecraft::client::render::entity::model {

class BoatEntityModel : public EntityModel {
public:
    std::array<net::minecraft::client::model::ModelPart, 5> parts;

    BoatEntityModel()
    {
        parts[0] = net::minecraft::client::model::ModelPart(0, 8);
        parts[1] = net::minecraft::client::model::ModelPart(0, 0);
        parts[2] = net::minecraft::client::model::ModelPart(0, 0);
        parts[3] = net::minecraft::client::model::ModelPart(0, 0);
        parts[4] = net::minecraft::client::model::ModelPart(0, 0);

        const int n = 24;
        const int n2 = 6;
        const int n3 = 20;
        const int n4 = 4;

        parts[0].addCuboid(static_cast<float>(-n / 2), static_cast<float>(-n3 / 2 + 2), -3.0f, n, n3 - 4, 4, 0.0f);
        parts[0].setPivot(0.0f, static_cast<float>(n4), 0.0f);

        parts[1].addCuboid(static_cast<float>(-n / 2 + 2), static_cast<float>(-n2 - 1), -1.0f, n - 4, n2, 2, 0.0f);
        parts[1].setPivot(static_cast<float>(-n / 2 + 1), static_cast<float>(n4), 0.0f);

        parts[2].addCuboid(static_cast<float>(-n / 2 + 2), static_cast<float>(-n2 - 1), -1.0f, n - 4, n2, 2, 0.0f);
        parts[2].setPivot(static_cast<float>(n / 2 - 1), static_cast<float>(n4), 0.0f);

        parts[3].addCuboid(static_cast<float>(-n / 2 + 2), static_cast<float>(-n2 - 1), -1.0f, n - 4, n2, 2, 0.0f);
        parts[3].setPivot(0.0f, static_cast<float>(n4), static_cast<float>(-n3 / 2 + 1));

        parts[4].addCuboid(static_cast<float>(-n / 2 + 2), static_cast<float>(-n2 - 1), -1.0f, n - 4, n2, 2, 0.0f);
        parts[4].setPivot(0.0f, static_cast<float>(n4), static_cast<float>(n3 / 2 - 1));

        parts[0].pitch = 1.5707964f;
        parts[1].yaw = 4.712389f;
        parts[2].yaw = 1.5707964f;
        parts[3].yaw = 3.14159265358979323846f;
    }

    void render(float limbAngle, float limbDistance, float animationProgress, float headYaw, float headPitch, float scale) override
    {
        (void)limbAngle;
        (void)limbDistance;
        (void)animationProgress;
        (void)headYaw;
        (void)headPitch;
        for (auto& part : parts) {
            part.render(scale);
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
};

} // namespace net::minecraft::client::render::entity::model
