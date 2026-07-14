#pragma once
#include <memory>
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
namespace net::minecraft {
}
namespace net::minecraft::client::render::entity {
// Faithful port of net.minecraft.client.render.entity.LivingEntityRenderer.
class LivingEntityRenderer : public EntityRenderer {
public:
  LivingEntityRenderer(model::EntityModel* model = nullptr, float shadowRadius = 0.0f);
  ~LivingEntityRenderer() override = default;
  void render(
      const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) override;
  void setDecorationModel(model::EntityModel* modelIn);

protected:
  std::unique_ptr<model::EntityModel> model;
  // Non-owning observer; the active decoration model is owned by the subclass.
  model::EntityModel* decorationModel = nullptr;
  virtual void applyTranslation(const net::minecraft::LivingEntity& entity, double x, double y, double z);
  virtual void applyHandSwingRotation(const net::minecraft::LivingEntity& entity,
                                      float headBob,
                                      float bodyYaw,
                                      float tickDelta);
  [[nodiscard]] virtual float getHandSwingProgress(const net::minecraft::LivingEntity& entity, float tickDelta) const;
  [[nodiscard]] virtual float getHeadBob(const net::minecraft::LivingEntity& entity, float tickDelta) const;
  virtual void renderMore(const net::minecraft::LivingEntity& entity, float tickDelta);
  [[nodiscard]] virtual bool bindDecorationTexture(const net::minecraft::LivingEntity& entity,
                                                   int layer,
                                                   float tickDelta);
  [[nodiscard]] virtual bool bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta);
  [[nodiscard]] virtual float getDeathYaw(const net::minecraft::LivingEntity& entity) const;
  [[nodiscard]] virtual int getOverlayColor(const net::minecraft::LivingEntity& entity,
                                            float brightness,
                                            float tickDelta) const;
  virtual void applyScale(const net::minecraft::LivingEntity& entity, float tickDelta);
  virtual void renderNameTag(const net::minecraft::LivingEntity& entity, double x, double y, double z);
  void renderNameTag(
      const net::minecraft::LivingEntity& entity, const std::string& name, double x, double y, double z, int range);
};
} // namespace net::minecraft::client::render::entity
