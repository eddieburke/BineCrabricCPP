#pragma once
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/QuadrupedEntityModel.hpp"
namespace net::minecraft::client::render::entity::model {
class SheepEntityModel : public QuadrupedEntityModel {
 public:
 SheepEntityModel() : QuadrupedEntityModel(12, 0.0f) {
  head = net::minecraft::client::model::ModelPart(0, 0);
  head.addCuboid(-3.0f, -4.0f, -6.0f, 6, 6, 8, 0.0f);
  head.setPivot(0.0f, 6.0f, -8.0f);
  body = net::minecraft::client::model::ModelPart(28, 8);
  body.addCuboid(-4.0f, -10.0f, -7.0f, 8, 16, 6, 0.0f);
  body.setPivot(0.0f, 5.0f, 2.0f);
 }
};
} // namespace net::minecraft::client::render::entity::model
