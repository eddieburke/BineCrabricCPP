#pragma once
#include "net/minecraft/client/model/ModelPart.hpp"
namespace net::minecraft::client::render::block::entity {
class SignModel {
 public:
 SignModel() {
  root.addCuboid(-12.0f, -14.0f, -1.0f, 24, 12, 2, 0.0f);
  stick.addCuboid(-1.0f, -2.0f, -1.0f, 2, 14, 2, 0.0f);
 }
 void render() {
  root.render(0.0625f);
  stick.render(0.0625f);
 }
 net::minecraft::client::model::ModelPart root{0, 0};
 net::minecraft::client::model::ModelPart stick{0, 14};
};
} // namespace net::minecraft::client::render::block::entity
