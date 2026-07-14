#pragma once
#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/BipedEntityModel.hpp"
namespace net::minecraft::client::render::entity {
class PlayerEntityRenderer : public LivingEntityRenderer {
public:
  PlayerEntityRenderer();
  void render(
      const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) override;
  void renderHand();

protected:
  bool bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) override;
  void applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) override;
  void applyTranslation(const net::minecraft::LivingEntity& entity, double x, double y, double z) override;
  void applyHandSwingRotation(const net::minecraft::LivingEntity& entity,
                              float headBob,
                              float bodyYaw,
                              float tickDelta) override;
  void renderNameTag(const net::minecraft::LivingEntity& entity, double x, double y, double z) override;
  void renderMore(const net::minecraft::LivingEntity& entity, float tickDelta) override;

private:
  model::BipedEntityModel* bipedModel = nullptr; // non-owning; aliases base `model`
  std::unique_ptr<model::BipedEntityModel> armor1;
  std::unique_ptr<model::BipedEntityModel> armor2;
};
} // namespace net::minecraft::client::render::entity
