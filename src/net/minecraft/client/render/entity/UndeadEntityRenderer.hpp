#pragma once
#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/BipedEntityModel.hpp"
namespace net::minecraft::client::render::entity {
class UndeadEntityRenderer : public LivingEntityRenderer {
 public:
 UndeadEntityRenderer(model::BipedEntityModel* model, float shadowSize);

 protected:
 void renderMore(const net::minecraft::LivingEntity& entity,
                 float tickDelta,
                 net::minecraft::util::math::MatrixStack& matrices,
                 const net::minecraft::util::math::Matrix4f& projection) override;

 private:
 model::BipedEntityModel* entityModel_ = nullptr;
};
} // namespace net::minecraft::client::render::entity
