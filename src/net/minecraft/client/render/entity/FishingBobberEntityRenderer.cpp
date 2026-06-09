#include "net/minecraft/client/render/entity/FishingBobberEntityRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Vec3dClient.hpp"

namespace net::minecraft::client::render::entity {

void FishingBobberEntityRenderer::render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw,
    float tickDelta)
{
    (void)yaw;
    const auto* bobber = dynamic_cast<const ::net::minecraft::entity::projectile::FishingBobberEntity*>(&entity);
    if (bobber == nullptr) {
        return;
    }

    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    gl::GL11::glEnable(32826);
    gl::GL11::glScalef(0.5f, 0.5f, 0.5f);
    constexpr int n = 1;
    constexpr int n2 = 2;
    bindTexture("/particles.png");
    Tessellator& tessellator = Tessellator::INSTANCE;
    const float f2 = static_cast<float>(n * 8 + 0) / 128.0f;
    const float f3 = static_cast<float>(n * 8 + 8) / 128.0f;
    const float f4 = static_cast<float>(n2 * 8 + 0) / 128.0f;
    const float f5 = static_cast<float>(n2 * 8 + 8) / 128.0f;
    constexpr float f6 = 1.0f;
    constexpr float f7 = 0.5f;
    constexpr float f8 = 0.5f;
    if (dispatcher != nullptr) {
        gl::GL11::glRotatef(180.0f - dispatcher->yaw_, 0.0f, 1.0f, 0.0f);
        gl::GL11::glRotatef(-dispatcher->pitch_, 1.0f, 0.0f, 0.0f);
    }
    tessellator.startQuads();
    tessellator.normal(0.0f, 1.0f, 0.0f);
    tessellator.vertex(0.0f - f7, 0.0f - f8, 0.0, f2, f5);
    tessellator.vertex(f6 - f7, 0.0f - f8, 0.0, f3, f5);
    tessellator.vertex(f6 - f7, f6 - f8, 0.0, f3, f4);
    tessellator.vertex(0.0f - f7, f6 - f8, 0.0, f2, f4);
    tessellator.draw();
    gl::GL11::glDisable(32826);
    gl::GL11::glPopMatrix();

    if (bobber->owner == nullptr || dispatcher == nullptr) {
        return;
    }

    const net::minecraft::LivingEntity& owner = *bobber->owner;
    float ownerYaw = (owner.prevYaw + (owner.yaw - owner.prevYaw) * tickDelta) * kPiF / 180.0f;
    double sinYaw = MathHelper::sin(ownerYaw);
    double cosYaw = MathHelper::cos(ownerYaw);
    const float swing = owner.getHandSwingProgress(tickDelta);
    const float swingBob = MathHelper::sin(MathHelper::sqrt(swing) * kPiF);

    util::math::ClientVec3d& vec = util::math::ClientVec3d::createCached(-0.5, 0.03, 0.8);
    vec.rotateX(-(owner.prevPitch + (owner.pitch - owner.prevPitch) * tickDelta) * kPiF / 180.0f);
    vec.rotateY(-(owner.prevYaw + (owner.yaw - owner.prevYaw) * tickDelta) * kPiF / 180.0f);
    vec.rotateY(swingBob * 0.5f);
    vec.rotateX(-swingBob * 0.7f);

    double d4 = owner.prevX + (owner.x - owner.prevX) * static_cast<double>(tickDelta) + vec.x;
    double d5 = owner.prevY + (owner.y - owner.prevY) * static_cast<double>(tickDelta) + vec.y;
    double d6 = owner.prevZ + (owner.z - owner.prevZ) * static_cast<double>(tickDelta) + vec.z;

    if (dispatcher->options().thirdPerson) {
        ownerYaw = (owner.lastBodyYaw + (owner.bodyYaw - owner.lastBodyYaw) * tickDelta) * kPiF / 180.0f;
        sinYaw = MathHelper::sin(ownerYaw);
        cosYaw = MathHelper::cos(ownerYaw);
        d4 = owner.prevX + (owner.x - owner.prevX) * static_cast<double>(tickDelta) - cosYaw * 0.35 - sinYaw * 0.85;
        d5 = owner.prevY + (owner.y - owner.prevY) * static_cast<double>(tickDelta) - 0.45;
        d6 = owner.prevZ + (owner.z - owner.prevZ) * static_cast<double>(tickDelta) - sinYaw * 0.35 + cosYaw * 0.85;
    }

    const double d7 = bobber->prevX + (bobber->x - bobber->prevX) * static_cast<double>(tickDelta);
    const double d8 = bobber->prevY + (bobber->y - bobber->prevY) * static_cast<double>(tickDelta) + 0.25;
    const double d9 = bobber->prevZ + (bobber->z - bobber->prevZ) * static_cast<double>(tickDelta);
    const double d10 = static_cast<float>(d4 - d7);
    const double d11 = static_cast<float>(d5 - d8);
    const double d12 = static_cast<float>(d6 - d9);

    gl::GL11::glDisable(3553);
    gl::GL11::glDisable(2896);
    tessellator.start(3); // GL_LINE_STRIP
    tessellator.color(0);
    constexpr int segments = 16;
    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        tessellator.vertex(x + d10 * static_cast<double>(t), y + d11 * static_cast<double>(t * t + t) * 0.5 + 0.25,
            z + d12 * static_cast<double>(t));
    }
    tessellator.draw();
    gl::GL11::glEnable(2896);
    gl::GL11::glEnable(3553);
}

} // namespace net::minecraft::client::render::entity
