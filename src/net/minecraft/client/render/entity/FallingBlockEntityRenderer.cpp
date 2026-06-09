#include "net/minecraft/client/render/entity/FallingBlockEntityRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::client::render::entity {

FallingBlockEntityRenderer::FallingBlockEntityRenderer()
{
    shadowRadius = 0.5f;
}

void FallingBlockEntityRenderer::render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw,
    float tickDelta)
{
    (void)yaw;
    (void)tickDelta;
    const auto* falling = dynamic_cast<const net::minecraft::FallingBlockEntity*>(&entity);
    if (falling == nullptr) {
        return;
    }

    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    bindTexture("/terrain.png");
    net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(falling->blockId)];
    if (block != nullptr) {
        gl::GL11::glDisable(2896);
        blockRenderManager_.renderFallingBlockEntity(
            *block, falling->world, MathHelper::floor(falling->x), MathHelper::floor(falling->y), MathHelper::floor(falling->z));
        gl::GL11::glEnable(2896);
    }
    gl::GL11::glPopMatrix();
}

} // namespace net::minecraft::client::render::entity
