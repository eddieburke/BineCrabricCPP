#include "net/minecraft/client/render/entity/ArrowEntityRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRendererCasts.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::client::render::entity {

void ArrowEntityRenderer::render(const net::minecraft::Entity& entity, double x, double y, double z, float /*yaw*/, float tickDelta)
{
    const auto* arrow = dynamic_cast<const casts::ArrowEntity*>(&entity);
    if (arrow == nullptr) {
        return;
    }
    if (arrow->prevYaw == 0.0f && arrow->prevPitch == 0.0f) {
        return;
    }

    bindTexture("/item/arrows.png");
    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    gl::GL11::glRotatef(arrow->prevYaw + (arrow->yaw - arrow->prevYaw) * tickDelta - 90.0f, 0.0f, 1.0f, 0.0f);
    gl::GL11::glRotatef(arrow->prevPitch + (arrow->pitch - arrow->prevPitch) * tickDelta, 0.0f, 0.0f, 1.0f);

    Tessellator& tessellator = Tessellator::INSTANCE;
    constexpr int n = 0;
    constexpr float f2 = 0.0f;
    constexpr float f3 = 0.5f;
    const float f4 = static_cast<float>(0 + n * 10) / 32.0f;
    const float f5 = static_cast<float>(5 + n * 10) / 32.0f;
    constexpr float f6 = 0.0f;
    constexpr float f7 = 0.15625f;
    const float f8 = static_cast<float>(5 + n * 10) / 32.0f;
    const float f9 = static_cast<float>(10 + n * 10) / 32.0f;
    constexpr float f10 = 0.05625f;

    gl::GL11::glEnable(32826);
    const float f11 = arrow->shake - tickDelta;
    if (f11 > 0.0f) {
        const float f12 = -MathHelper::sin(f11 * 3.0f) * f11;
        gl::GL11::glRotatef(f12, 0.0f, 0.0f, 1.0f);
    }
    gl::GL11::glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
    gl::GL11::glScalef(f10, f10, f10);
    gl::GL11::glTranslatef(-4.0f, 0.0f, 0.0f);
    gl::GL11::glNormal3f(f10, 0.0f, 0.0f);
    tessellator.startQuads();
    tessellator.vertex(-7.0, -2.0, -2.0, f6, f8);
    tessellator.vertex(-7.0, -2.0, 2.0, f7, f8);
    tessellator.vertex(-7.0, 2.0, 2.0, f7, f9);
    tessellator.vertex(-7.0, 2.0, -2.0, f6, f9);
    tessellator.draw();
    gl::GL11::glNormal3f(-f10, 0.0f, 0.0f);
    tessellator.startQuads();
    tessellator.vertex(-7.0, 2.0, -2.0, f6, f8);
    tessellator.vertex(-7.0, 2.0, 2.0, f7, f8);
    tessellator.vertex(-7.0, -2.0, 2.0, f7, f9);
    tessellator.vertex(-7.0, -2.0, -2.0, f6, f9);
    tessellator.draw();
    for (int i = 0; i < 4; ++i) {
        gl::GL11::glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gl::GL11::glNormal3f(0.0f, 0.0f, f10);
        tessellator.startQuads();
        tessellator.vertex(-8.0, -2.0, 0.0, f2, f4);
        tessellator.vertex(8.0, -2.0, 0.0, f3, f4);
        tessellator.vertex(8.0, 2.0, 0.0, f3, f5);
        tessellator.vertex(-8.0, 2.0, 0.0, f2, f5);
        tessellator.draw();
    }
    gl::GL11::glDisable(32826);
    gl::GL11::glPopMatrix();
}

} // namespace net::minecraft::client::render::entity
