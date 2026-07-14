#pragma once
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/QuadrupedEntityModel.hpp"
namespace net::minecraft::client::render::entity::model {
class SheepWoolEntityModel : public QuadrupedEntityModel {
public:
  SheepWoolEntityModel() : QuadrupedEntityModel(12, 0.0f) {
    head = net::minecraft::client::model::ModelPart(0, 0);
    head.addCuboid(-3.0f, -4.0f, -4.0f, 6, 6, 6, 0.6f);
    head.setPivot(0.0f, 6.0f, -8.0f);
    body = net::minecraft::client::model::ModelPart(28, 8);
    body.addCuboid(-4.0f, -10.0f, -7.0f, 8, 16, 6, 1.75f);
    body.setPivot(0.0f, 5.0f, 2.0f);
    const float legScale = 0.5f;
    rightHindLeg = net::minecraft::client::model::ModelPart(0, 16);
    rightHindLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, legScale);
    rightHindLeg.setPivot(-3.0f, 12.0f, 7.0f);
    leftHindLeg = net::minecraft::client::model::ModelPart(0, 16);
    leftHindLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, legScale);
    leftHindLeg.setPivot(3.0f, 12.0f, 7.0f);
    rightFrontLeg = net::minecraft::client::model::ModelPart(0, 16);
    rightFrontLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, legScale);
    rightFrontLeg.setPivot(-3.0f, 12.0f, -5.0f);
    leftFrontLeg = net::minecraft::client::model::ModelPart(0, 16);
    leftFrontLeg.addCuboid(-2.0f, 0.0f, -2.0f, 4, 6, 4, legScale);
    leftFrontLeg.setPivot(3.0f, 12.0f, -5.0f);
  }
};
} // namespace net::minecraft::client::render::entity::model
