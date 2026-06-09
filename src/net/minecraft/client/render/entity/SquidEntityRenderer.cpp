#include "net/minecraft/client/render/entity/SquidEntityRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/entity/EntityRendererCasts.hpp"

namespace net::minecraft::client::render::entity {

void SquidEntityRenderer::applyHandSwingRotation(const net::minecraft::LivingEntity& entity, float headBob, float bodyYaw,
    float tickDelta)
{
    const auto* squid = dynamic_cast<const casts::SquidEntity*>(&entity);
    if (squid == nullptr) {
        LivingEntityRenderer::applyHandSwingRotation(entity, headBob, bodyYaw, tickDelta);
        return;
    }
    const float tilt = squid->lastTiltAngle + (squid->tiltAngle - squid->lastTiltAngle) * tickDelta;
    const float roll = squid->lastRollAngle + (squid->rollAngle - squid->lastRollAngle) * tickDelta;
    gl::GL11::glTranslatef(0.0f, 0.5f, 0.0f);
    gl::GL11::glRotatef(180.0f - bodyYaw, 0.0f, 1.0f, 0.0f);
    gl::GL11::glRotatef(tilt, 1.0f, 0.0f, 0.0f);
    gl::GL11::glRotatef(roll, 0.0f, 1.0f, 0.0f);
    gl::GL11::glTranslatef(0.0f, -1.2f, 0.0f);
}

void SquidEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta)
{
    (void)entity;
    (void)tickDelta;
}

float SquidEntityRenderer::getHeadBob(const net::minecraft::LivingEntity& entity, float tickDelta) const
{
    const auto* squid = dynamic_cast<const casts::SquidEntity*>(&entity);
    if (squid == nullptr) {
        return LivingEntityRenderer::getHeadBob(entity, tickDelta);
    }
    return squid->lastTentacleAngle + (squid->tentacleAngle - squid->lastTentacleAngle) * tickDelta;
}

} // namespace net::minecraft::client::render::entity
