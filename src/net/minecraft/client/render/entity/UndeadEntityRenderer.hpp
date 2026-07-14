#pragma once
#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/BipedEntityModel.hpp"
namespace net::minecraft::client::render::entity {
class UndeadEntityRenderer : public LivingEntityRenderer {
public:
  UndeadEntityRenderer(model::BipedEntityModel* model, float shadowSize);

protected:
  void renderMore(const net::minecraft::LivingEntity& entity, float tickDelta) override;

private:
  model::BipedEntityModel* entityModel_ = nullptr;
};
} // namespace net::minecraft::client::render::entity
