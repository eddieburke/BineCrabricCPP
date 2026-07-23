#pragma once
#include <array>
#include <cmath>
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
namespace net::minecraft::client::render::entity::model {
class MinecartEntityModel : public EntityModel {
 public:
 std::array<net::minecraft::client::model::ModelPart, 7> parts;
 MinecartEntityModel() {
  parts[0] = net::minecraft::client::model::ModelPart(0, 10);
  parts[1] = net::minecraft::client::model::ModelPart(0, 0);
  parts[2] = net::minecraft::client::model::ModelPart(0, 0);
  parts[3] = net::minecraft::client::model::ModelPart(0, 0);
  parts[4] = net::minecraft::client::model::ModelPart(0, 0);
  parts[5] = net::minecraft::client::model::ModelPart(44, 10);
  constexpr int bodyWidth = 20;
  constexpr int wheelHeight = 8;
  constexpr int bodyLength = 16;
  constexpr int bodyPivotY = 4;
  parts[0].addCuboid(static_cast<float>(-bodyWidth / 2),
                     static_cast<float>(-bodyLength / 2),
                     -1.0f,
                     bodyWidth,
                     bodyLength,
                     2,
                     0.0f);
  parts[0].setPivot(0.0f, static_cast<float>(bodyPivotY), 0.0f);
  parts[5].addCuboid(static_cast<float>(-bodyWidth / 2 + 1),
                     static_cast<float>(-bodyLength / 2 + 1),
                     -1.0f,
                     bodyWidth - 2,
                     bodyLength - 2,
                     1,
                     0.0f);
  parts[5].setPivot(0.0f, static_cast<float>(bodyPivotY), 0.0f);
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
  parts[5].pitch = -1.5707964f;
 }
 void setAngles(float, float, float animationProgress, float, float, float) override {
  parts[5].pivotY = 4.0f - animationProgress;
  applyPartOverrides();
 }
 void render(float limbAngle,
             float limbDistance,
             float animationProgress,
             float headYaw,
             float headPitch,
             float scale) override {
  setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
  for(int i = 0; i < 6; ++i) {
   parts[static_cast<std::size_t>(i)].render(scale);
  }
 }
 void collectNamedParts(std::unordered_map<std::string, net::minecraft::client::model::ModelPart*>& parts) override {
  constexpr const char* names[] = {"bottom", "leftWall", "rightWall", "frontWall", "backWall", "top"};
  for(std::size_t i = 0; i < 6; ++i) {
   parts[names[i]] = &this->parts[i];
  }
 }
};
} // namespace net::minecraft::client::render::entity::model
