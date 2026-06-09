#include "net/minecraft/client/render/entity/SlimeEntityRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/entity/EntityRendererCasts.hpp"

namespace net::minecraft::client::render::entity {

SlimeEntityRenderer::SlimeEntityRenderer(model::EntityModel* model, model::EntityModel* innerModel, float shadowRadius)
    : LivingEntityRenderer(model, shadowRadius)
{
    setDecorationModel(innerModel);
}

bool SlimeEntityRenderer::bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta)
{
    (void)entity;
    (void)tickDelta;
    if (layer == 0) {
        gl::GL11::glEnable(2977);
        gl::GL11::glEnable(3042);
        gl::GL11::glBlendFunc(770, 771);
        return true;
    }
    if (layer == 1) {
        gl::GL11::glDisable(3042);
        gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    return false;
}

void SlimeEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta)
{
    const auto* slime = dynamic_cast<const casts::SlimeEntity*>(&entity);
    if (slime == nullptr) {
        return;
    }
    const int size = slime->getSize();
    const float stretch =
        (slime->lastStretch + (slime->stretch - slime->lastStretch) * tickDelta) / (static_cast<float>(size) * 0.5f + 1.0f);
    const float invStretch = 1.0f / (stretch + 1.0f);
    const float sizeScale = static_cast<float>(size);
    gl::GL11::glScalef(invStretch * sizeScale, (1.0f / invStretch) * sizeScale, invStretch * sizeScale);
}

} // namespace net::minecraft::client::render::entity
