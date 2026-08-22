#pragma once
#include <string>
#include <string_view>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/util/math/MatrixStack.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::font {
class TextRenderer;
}
namespace net::minecraft::client::render::entity {
class EntityRenderDispatcher;
// Faithful port of net.minecraft.client.render.entity.EntityRenderer (beta 1.7.3).
class EntityRenderer {
 public:
 virtual ~EntityRenderer() = default;
 EntityRenderDispatcher* dispatcher = nullptr;
 float shadowRadius = 0.0f;
 float shadowDarkness = 1.0f;
 virtual void render(const net::minecraft::Entity& entity,
                     double x,
                     double y,
                     double z,
                     float yaw,
                     float tickDelta,
                     net::minecraft::util::math::MatrixStack& matrices,
                     const net::minecraft::util::math::Matrix4f& projection) = 0;
 virtual void setDispatcher(EntityRenderDispatcher* dispatcherIn);
 virtual void postRender(const net::minecraft::Entity& entity,
                         double x,
                         double y,
                         double z,
                         float yaw,
                         float tickDelta,
                         net::minecraft::util::math::MatrixStack& matrices,
                         const net::minecraft::util::math::Matrix4f& projection);
 void bindTexture(std::string_view texturePath);
 [[nodiscard]] bool bindDownloadedTexture(std::string_view url, std::string_view backup = "");
 static void renderShape(const Box& box, double x, double y, double z);
 [[nodiscard]] font::TextRenderer* getTextRenderer() const noexcept;

 protected:
 // Active for the duration of render()/postRender(); subclasses must not stash across frames.
 net::minecraft::util::math::MatrixStack* matrices_ = nullptr;
 const net::minecraft::util::math::Matrix4f* projection_ = nullptr;
 void beginDraw(net::minecraft::util::math::MatrixStack& matrices,
                const net::minecraft::util::math::Matrix4f& projection) {
  matrices_ = &matrices;
  projection_ = &projection;
 }
 void endDraw() {
  matrices_ = nullptr;
  projection_ = nullptr;
 }
 [[nodiscard]] net::minecraft::util::math::MatrixStack& matrices() {
  return *matrices_;
 }
 [[nodiscard]] const net::minecraft::util::math::Matrix4f& projection() const {
  return *projection_;
 }

 private:
 void renderOnFire(const net::minecraft::Entity& entity,
                   double dx,
                   double dy,
                   double dz,
                   float tickDelta,
                   net::minecraft::util::math::MatrixStack& matrices);
 void renderShadow(const net::minecraft::Entity& entity,
                   double dx,
                   double dy,
                   double dz,
                   float yaw,
                   float tickDelta,
                   net::minecraft::util::math::MatrixStack& matrices);
 [[nodiscard]] net::minecraft::World* getWorld() const;
 void renderShadowOnBlock(net::minecraft::block::Block& block,
                          double dx,
                          double dy,
                          double dz,
                          int x,
                          int y,
                          int z,
                          float yaw,
                          float shadowSize,
                          double cx,
                          double cy,
                          double cz);
};
} // namespace net::minecraft::client::render::entity
