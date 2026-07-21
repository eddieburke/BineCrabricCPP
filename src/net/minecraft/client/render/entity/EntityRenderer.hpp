#pragma once
#include <string>
#include <string_view>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/entity/Entity.hpp"
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
 virtual void render(
     const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) = 0;
 virtual void setDispatcher(EntityRenderDispatcher* dispatcherIn);
 virtual void postRender(
     const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta);
 void bindTexture(std::string_view texturePath);
 [[nodiscard]] bool bindDownloadedTexture(std::string_view url);
 static void renderShape(const Box& box, double x, double y, double z);
 static void renderShapeFlat(const Box& box);
 [[nodiscard]] font::TextRenderer* getTextRenderer() const noexcept;

 private:
 void renderOnFire(const net::minecraft::Entity& entity, double dx, double dy, double dz, float tickDelta);
 void renderShadow(
     const net::minecraft::Entity& entity, double dx, double dy, double dz, float yaw, float tickDelta);
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
