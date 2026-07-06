#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/entity/Entity.hpp"
namespace net::minecraft::client::render::entity {
void BoxEntityRenderer::render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw,
                               float tickDelta) {
  (void)yaw;
  (void)tickDelta;
  gl::GL11::glPushMatrix();
  EntityRenderer::renderShape(entity.boundingBox, x - entity.lastTickX, y - entity.lastTickY, z - entity.lastTickZ);
  gl::GL11::glPopMatrix();
}
} // namespace net::minecraft::client::render::entity
