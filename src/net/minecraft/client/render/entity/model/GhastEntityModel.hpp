#pragma once
#include <array>
#include <cmath>
#include "net/minecraft/client/model/ModelPart.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render::entity::model {
class GhastEntityModel : public EntityModel {
public:
  net::minecraft::client::model::ModelPart root;
  std::array<net::minecraft::client::model::ModelPart, 9> tentacles;
  GhastEntityModel() {
    constexpr int bodyYOffset = -16;
    root = net::minecraft::client::model::ModelPart(0, 0);
    root.addCuboid(-8.0f, -8.0f, -8.0f, 16, 16, 16);
    root.pivotY += static_cast<float>(24 + bodyYOffset);
    net::minecraft::JavaRandom random(1660ULL);
    for(std::size_t i = 0; i < tentacles.size(); ++i) {
      tentacles[i] = net::minecraft::client::model::ModelPart(0, 0);
      const float offsetX =
          (((static_cast<float>(i % 3) - static_cast<float>((i / 3) % 2) * 0.5f + 0.25f) / 2.0f * 2.0f - 1.0f) *
           5.0f);
      const float offsetZ = ((static_cast<float>(i / 3) / 2.0f * 2.0f - 1.0f) * 5.0f);
      const int tentacleLength = random.nextInt(7) + 8;
      tentacles[i].addCuboid(-1.0f, 0.0f, -1.0f, 2, tentacleLength, 2);
      tentacles[i].pivotX = offsetX;
      tentacles[i].pivotZ = offsetZ;
      tentacles[i].pivotY = static_cast<float>(31 + bodyYOffset);
    }
  }
  void setAngles(float limbAngle,
                 float limbDistance,
                 float animationProgress,
                 float headYaw,
                 float headPitch,
                 float scale) override {
    (void)limbAngle;
    (void)limbDistance;
    (void)headYaw;
    (void)headPitch;
    (void)scale;
    for(std::size_t i = 0; i < tentacles.size(); ++i) {
      tentacles[i].pitch =
          0.2f * net::minecraft::util::math::MathHelper::sin(animationProgress * 0.3f + static_cast<float>(i)) +
          0.4f;
    }
  }
  void render(float limbAngle,
              float limbDistance,
              float animationProgress,
              float headYaw,
              float headPitch,
              float scale) override {
    setAngles(limbAngle, limbDistance, animationProgress, headYaw, headPitch, scale);
    root.render(scale);
    for(auto& tentacle : tentacles) {
      tentacle.render(scale);
    }
  }
};
} // namespace net::minecraft::client::render::entity::model
