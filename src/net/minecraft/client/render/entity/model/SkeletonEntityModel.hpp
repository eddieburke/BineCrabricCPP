#pragma once

#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/ZombieEntityModel.hpp"

namespace net::minecraft::client::render::entity::model {

class SkeletonEntityModel : public ZombieEntityModel {
public:
    SkeletonEntityModel()
    {
        const float f = 0.0f;
        rightArm = net::minecraft::client::model::ModelPart(40, 16);
        rightArm.addCuboid(-1.0f, -2.0f, -1.0f, 2, 12, 2, f);
        rightArm.setPivot(-5.0f, 2.0f, 0.0f);

        leftArm = net::minecraft::client::model::ModelPart(40, 16);
        leftArm.mirror = true;
        leftArm.addCuboid(-1.0f, -2.0f, -1.0f, 2, 12, 2, f);
        leftArm.setPivot(5.0f, 2.0f, 0.0f);

        rightLeg = net::minecraft::client::model::ModelPart(0, 16);
        rightLeg.addCuboid(-1.0f, 0.0f, -1.0f, 2, 12, 2, f);
        rightLeg.setPivot(-2.0f, 12.0f, 0.0f);

        leftLeg = net::minecraft::client::model::ModelPart(0, 16);
        leftLeg.mirror = true;
        leftLeg.addCuboid(-1.0f, 0.0f, -1.0f, 2, 12, 2, f);
        leftLeg.setPivot(2.0f, 12.0f, 0.0f);
    }
};

} // namespace net::minecraft::client::render::entity::model
