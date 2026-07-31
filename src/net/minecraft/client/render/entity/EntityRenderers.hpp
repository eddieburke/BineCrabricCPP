#pragma once
#include <random>
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render::entity::model {
class EntityModel;
}
namespace net::minecraft::entity::decoration::painting {
class PaintingEntity;
}
namespace net::minecraft::client::render::entity {
class ArrowEntityRenderer : public EntityRenderer {
 public:
 using EntityRenderer::EntityRenderer;
 void render(const net::minecraft::Entity&, double, double, double, float, float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override;
};
class BoxEntityRenderer : public EntityRenderer {
 public:
 using EntityRenderer::EntityRenderer;
 void render(const net::minecraft::Entity& entity, double x, double y, double z, float /*yaw*/, float /*tickDelta*/,
             net::minecraft::util::math::MatrixStack& matrices,
             const net::minecraft::util::math::Matrix4f& projection) override {
  beginDraw(matrices, projection);
  matrices.push();
  renderShape(entity.boundingBox, x - entity.lastTickX, y - entity.lastTickY, z - entity.lastTickZ);
  matrices.pop();
  endDraw();
 }
};
class FireballEntityRenderer : public EntityRenderer {
 public:
 using EntityRenderer::EntityRenderer;
 void render(const net::minecraft::Entity&, double, double, double, float, float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override;
};
class FishingBobberEntityRenderer : public EntityRenderer {
 public:
 using EntityRenderer::EntityRenderer;
 void render(const net::minecraft::Entity&, double, double, double, float, float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override;
};
class LightningEntityRenderer : public EntityRenderer {
 public:
 using EntityRenderer::EntityRenderer;
 void render(const net::minecraft::Entity&, double, double, double, float, float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override;
};
class BoatEntityRenderer : public EntityRenderer {
 public:
 BoatEntityRenderer();
 ~BoatEntityRenderer() override;
 void render(const net::minecraft::Entity&, double, double, double, float, float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override;

 private:
 model::EntityModel* model_ = nullptr;
};
class FallingBlockEntityRenderer : public EntityRenderer {
 public:
 FallingBlockEntityRenderer();
 void render(const net::minecraft::Entity&, double, double, double, float, float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override;

 private:
 block::BlockRenderManager blockRenderManager_;
};
class ItemEntityRenderer : public EntityRenderer {
 public:
 ItemEntityRenderer();
 void render(const net::minecraft::Entity&, double, double, double, float, float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override;

 private:
 block::BlockRenderManager blockRenderManager_;
 JavaRandom random_{187L};
 bool useCustomDisplayColor_ = true;
};
class MinecartEntityRenderer : public EntityRenderer {
 public:
 MinecartEntityRenderer();
 ~MinecartEntityRenderer() override;
 void render(const net::minecraft::Entity&, double, double, double, float, float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override;

 private:
 model::EntityModel* model_ = nullptr;
 block::BlockRenderManager blockRenderManager_;
};
class TntEntityRenderer : public EntityRenderer {
 public:
 TntEntityRenderer();
 void render(const net::minecraft::Entity&, double, double, double, float, float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override;

 private:
 block::BlockRenderManager blockRenderManager_;
};
class PaintingEntityRenderer : public EntityRenderer {
 public:
 void render(const net::minecraft::Entity&, double, double, double, float, float,
             net::minecraft::util::math::MatrixStack&,
             const net::minecraft::util::math::Matrix4f&) override;

 private:
 void renderPainting(const ::net::minecraft::entity::decoration::painting::PaintingEntity&, int, int, int, int);
 void applyBrightness(const ::net::minecraft::entity::decoration::painting::PaintingEntity&, float, float);
 std::mt19937_64 random_{187ULL};
};
class ChickenEntityRenderer : public LivingEntityRenderer {
 public:
 using LivingEntityRenderer::LivingEntityRenderer;

 protected:
 [[nodiscard]] float getHeadBob(const net::minecraft::LivingEntity&, float) const override;
};
class CowEntityRenderer : public LivingEntityRenderer {
 public:
 CowEntityRenderer(model::EntityModel*, float);
};
class CreeperEntityRenderer : public LivingEntityRenderer {
 public:
 CreeperEntityRenderer();
 ~CreeperEntityRenderer() override;

 protected:
 void applyScale(const net::minecraft::LivingEntity&,
                 float,
                 net::minecraft::util::math::MatrixStack& matrices) override;
 int getOverlayColor(const net::minecraft::LivingEntity&, float, float) const override;
 bool bindTexture(const net::minecraft::LivingEntity&, int, float) override;
 bool bindDecorationTexture(const net::minecraft::LivingEntity&, int, float) override;

 private:
 model::EntityModel* chargedModel_ = nullptr;
};
class GhastEntityRenderer : public LivingEntityRenderer {
 public:
 GhastEntityRenderer();

 protected:
 void applyScale(const net::minecraft::LivingEntity&,
                 float,
                 net::minecraft::util::math::MatrixStack& matrices) override;
};
class GiantEntityRenderer : public LivingEntityRenderer {
 public:
 GiantEntityRenderer(model::EntityModel*, float, float);

 protected:
 void applyScale(const net::minecraft::LivingEntity&,
                 float,
                 net::minecraft::util::math::MatrixStack& matrices) override;

 private:
 float scale_ = 1.0f;
};
class PigEntityRenderer : public LivingEntityRenderer {
 public:
 PigEntityRenderer(model::EntityModel*, model::EntityModel*, float);

 protected:
 bool bindTexture(const net::minecraft::LivingEntity&, int, float) override;
};
class SheepEntityRenderer : public LivingEntityRenderer {
 public:
 SheepEntityRenderer(model::EntityModel*, model::EntityModel*, float);

 protected:
 bool bindTexture(const net::minecraft::LivingEntity&, int, float) override;
};
class SlimeEntityRenderer : public LivingEntityRenderer {
 public:
 SlimeEntityRenderer(model::EntityModel*, model::EntityModel*, float);

 protected:
 [[nodiscard]] bool bindTexture(const net::minecraft::LivingEntity&, int, float) override;
 void applyScale(const net::minecraft::LivingEntity&,
                 float,
                 net::minecraft::util::math::MatrixStack& matrices) override;

 private:
 model::EntityModel* innerModel_ = nullptr;
};
class SpiderEntityRenderer : public LivingEntityRenderer {
 public:
 SpiderEntityRenderer();

 protected:
 float getDeathYaw(const net::minecraft::LivingEntity&) const override;
 bool bindTexture(const net::minecraft::LivingEntity&, int, float) override;
};
class SquidEntityRenderer : public LivingEntityRenderer {
 public:
 using LivingEntityRenderer::LivingEntityRenderer;

 protected:
 void applyHandSwingRotation(const net::minecraft::LivingEntity&,
                             float,
                             float,
                             float,
                             net::minecraft::util::math::MatrixStack& matrices) override;
 void applyScale(const net::minecraft::LivingEntity&,
                 float,
                 net::minecraft::util::math::MatrixStack& matrices) override;
 [[nodiscard]] float getHeadBob(const net::minecraft::LivingEntity&, float) const override;
};
class WolfEntityRenderer : public LivingEntityRenderer {
 public:
 WolfEntityRenderer(model::EntityModel*, float);

 protected:
 float getHeadBob(const net::minecraft::LivingEntity&, float) const override;
 void applyScale(const net::minecraft::LivingEntity&,
                 float,
                 net::minecraft::util::math::MatrixStack& matrices) override;
};
} // namespace net::minecraft::client::render::entity
