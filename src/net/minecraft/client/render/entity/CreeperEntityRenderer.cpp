#include "net/minecraft/client/render/entity/CreeperEntityRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/entity/model/CreeperEntityModel.hpp"
#include "net/minecraft/entity/mob/CreeperEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

#include <algorithm>

namespace net::minecraft::client::render::entity {

CreeperEntityRenderer::CreeperEntityRenderer()
    : LivingEntityRenderer(new model::CreeperEntityModel(), 0.5f),
      chargedModel(new model::CreeperEntityModel(2.0f))
{
}

CreeperEntityRenderer::~CreeperEntityRenderer()
{
    delete chargedModel;
    chargedModel = nullptr;
}

void CreeperEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta)
{
    const auto* creeper = dynamic_cast<const ::net::minecraft::entity::mob::CreeperEntity*>(&entity);
    float swell = creeper != nullptr ? creeper->getScale(tickDelta) : 0.0f;
    float pulse = 1.0f + MathHelper::sin(swell * 100.0f) * swell * 0.01f;
    if (swell < 0.0f) {
        swell = 0.0f;
    }
    if (swell > 1.0f) {
        swell = 1.0f;
    }
    swell *= swell;
    swell *= swell;
    const float sx = (1.0f + swell * 0.4f) * pulse;
    const float sy = (1.0f + swell * 0.1f) / pulse;
    gl::GL11::glScalef(sx, sy, sx);
}

int CreeperEntityRenderer::getOverlayColor(const net::minecraft::LivingEntity& entity, float brightness,
    float tickDelta) const
{
    (void)brightness;
    const auto* creeper = dynamic_cast<const ::net::minecraft::entity::mob::CreeperEntity*>(&entity);
    const float swell = creeper != nullptr ? creeper->getScale(tickDelta) : 0.0f;
    if (static_cast<int>(swell * 10.0f) % 2 == 0) {
        return 0;
    }
    int alpha = static_cast<int>(swell * 0.2f * 255.0f);
    alpha = std::clamp(alpha, 0, 255);
    return (alpha << 24) | (255 << 16) | (255 << 8) | 255;
}

bool CreeperEntityRenderer::bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta)
{
    const auto* creeper = dynamic_cast<const ::net::minecraft::entity::mob::CreeperEntity*>(&entity);
    if (creeper == nullptr || !creeper->isCharged()) {
        return false;
    }
    if (layer == 1) {
        const float time = static_cast<float>(entity.age) + tickDelta;
        EntityRenderer::bindTexture("/armor/power.png");
        gl::GL11::glMatrixMode(5890);
        gl::GL11::glLoadIdentity();
        const float offset = time * 0.01f;
        gl::GL11::glTranslatef(offset, offset, 0.0f);
        setDecorationModel(chargedModel);
        gl::GL11::glMatrixMode(5888);
        gl::GL11::glEnable(gl::GL11::GL_BLEND);
        gl::GL11::glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
        gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
        gl::GL11::glBlendFunc(gl::GL11::GL_ONE, gl::GL11::GL_ONE);
        return true;
    }
    if (layer == 2) {
        gl::GL11::glMatrixMode(5890);
        gl::GL11::glLoadIdentity();
        gl::GL11::glMatrixMode(5888);
        gl::GL11::glEnable(gl::GL11::GL_LIGHTING);
        gl::GL11::glDisable(gl::GL11::GL_BLEND);
    }
    return false;
}

bool CreeperEntityRenderer::bindDecorationTexture(const net::minecraft::LivingEntity& entity, int layer,
    float tickDelta)
{
    (void)entity;
    (void)layer;
    (void)tickDelta;
    return false;
}

} // namespace net::minecraft::client::render::entity
