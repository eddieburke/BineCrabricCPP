#pragma once
#include <array>
#include <cmath>
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
namespace net::minecraft::client::render::entity::model {
class BoatEntityModel : public EntityModel {
 public:
 std::array<net::minecraft::client::model::ModelPart, 5> parts;
 BoatEntityModel() {
  parts[0] = net::minecraft::client::model::ModelPart(0, 8);
  parts[1] = net::minecraft::client::model::ModelPart(0, 0);
  parts[2] = net::minecraft::client::model::ModelPart(0, 0);
  parts[3] = net::minecraft::client::model::ModelPart(0, 0);
  parts[4] = net::minecraft::client::model::ModelPart(0, 0);
  constexpr int bodyWidth = 24;
  constexpr int wheelHeight = 6;
  constexpr int bodyLength = 20;
  constexpr int bodyPivotY = 4;
  parts[0].addCuboid(static_cast<float>(-bodyWidth / 2),
                     static_cast<float>(-bodyLength / 2 + 2),
                     -3.0f,
                     bodyWidth,
                     bodyLength - 4,
                     4,
                     0.0f);
  parts[0].setPivot(0.0f, static_cast<float>(bodyPivotY), 0.0f);
  parts[1].addCuboid(static_cast<float>(-bodyWidth / 2 + 2),
                     static_cast<float>(-wheelHeight - 1),
                     -1.0f,
                     bodyWidth - 4,
                     wheelHeight,
                     2,
                     0.0f);
  parts[1].setPivot(static_cast<float>(-bodyWidth / 2 + 1), static_cast<float>(bodyPivotY), 0.0f);
  parts[2].addCuboid(static_cast<float>(-bodyWidth / 2 + 2),
                     static_cast<float>(-wheelHeight - 1),
                     -1.0f,
                     bodyWidth - 4,
                     wheelHeight,
                     2,
                     0.0f);
  parts[2].setPivot(static_cast<float>(bodyWidth / 2 - 1), static_cast<float>(bodyPivotY), 0.0f);
  parts[3].addCuboid(static_cast<float>(-bodyWidth / 2 + 2),
                     static_cast<float>(-wheelHeight - 1),
                     -1.0f,
                     bodyWidth - 4,
                     wheelHeight,
                     2,
                     0.0f);
  parts[3].setPivot(0.0f, static_cast<float>(bodyPivotY), static_cast<float>(-bodyLength / 2 + 1));
  parts[4].addCuboid(static_cast<float>(-bodyWidth / 2 + 2),
                     static_cast<float>(-wheelHeight - 1),
                     -1.0f,
                     bodyWidth - 4,
                     wheelHeight,
                     2,
                     0.0f);
  parts[4].setPivot(0.0f, static_cast<float>(bodyPivotY), static_cast<float>(bodyLength / 2 - 1));
  parts[0].pitch = 1.5707964f;
  parts[1].yaw = 4.712389f;
  parts[2].yaw = 1.5707964f;
  parts[3].yaw = 3.14159265358979323846f;
 }
 void setAngles(float, float, float, float, float, float) override {
  applyPartOverrides();
 }
 void render(float limbAngle,
             float limbDistance,
             float animationProgress,
             float headYaw,
             float headPitch,
             float scale) override {
  setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
  for(auto& part : parts) {
   part.render(scale);
  }
 }
 void collectNamedParts(std::unordered_map<std::string, net::minecraft::client::model::ModelPart*>& parts) override {
  constexpr const char* names[] = {"bottom", "leftWall", "rightWall", "frontWall", "backWall"};
  for(std::size_t i = 0; i < this->parts.size(); ++i) {
   parts[names[i]] = &this->parts[i];
  }
 }
};
} // namespace net::minecraft::client::render::entity::model
