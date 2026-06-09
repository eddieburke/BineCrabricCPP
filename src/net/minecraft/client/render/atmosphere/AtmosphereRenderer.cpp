#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/platform/GlState.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"

namespace net::minecraft::client::render::atmosphere {

namespace {
constexpr int kGlTriangleFan = 6;
constexpr float kPi = 3.14159265f;
} // namespace

void AtmosphereRenderer::rebuildStaticGeometry()
{
    starField_.build();
    skyDome_.build();
}

void AtmosphereRenderer::releaseGpuResources()
{
    starField_.release();
    skyDome_.release();
}

void AtmosphereRenderer::updateClearAndFogColors(const AtmosphereContext& ctx, float tickDelta)
{
    fog_.updateClearAndFogColors(ctx, tickDelta);
}

void AtmosphereRenderer::applySkyFog(const AtmosphereContext& ctx)
{
    fog_.applySkyFog(ctx);
}

void AtmosphereRenderer::applyWorldFog(const AtmosphereContext& ctx)
{
    fog_.applyWorldFog(ctx);
}

void AtmosphereRenderer::applyHandFog(const AtmosphereContext& ctx)
{
    fog_.applyHandFog(ctx);
}

void AtmosphereRenderer::renderSky(const AtmosphereContext& ctx, float tickDelta)
{
    if (!client::option::resolve(ctx.options).renderSky) {
        return;
    }
    if (ctx.world == nullptr || ctx.world->dimension == nullptr
        || ctx.textureManager == nullptr || ctx.camera == nullptr
        || ctx.world->dimension->isNether) {
        return;
    }

    platform::GlState::beginSkyPass();
    gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
    Vec3d skyColor = ctx.world->getSkyColor(ctx.camera, tickDelta);
    float red = static_cast<float>(skyColor.x);
    float green = static_cast<float>(skyColor.y);
    float blue = static_cast<float>(skyColor.z);

    gl::GL11::glColor3f(red, green, blue);
    gl::GL11::glDepthMask(false);
    platform::GlState::setFogEnabled(true);
    gl::GL11::glColor3f(red, green, blue);
    skyDome_.drawLight();
    platform::GlState::setFogEnabled(false);

    gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
    gl::GL11::glEnable(gl::GL11::GL_BLEND);
    gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
    platform::Lighting::turnOff();

    const float timeOfDay = ctx.world->getTime(tickDelta);
    std::array<float, 4>* background = ctx.world->dimension->getBackgroundColor(timeOfDay, tickDelta);
    if (background != nullptr) {
        gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
        gl::GL11::glShadeModel(gl::GL11::GL_SMOOTH);
        gl::GL11::glPushMatrix();
        gl::GL11::glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gl::GL11::glRotatef(timeOfDay > 0.5f ? 180.0f : 0.0f, 0.0f, 0.0f, 1.0f);
        float sunRed = (*background)[0];
        float sunGreen = (*background)[1];
        float sunBlue = (*background)[2];
        Tessellator& tessellator = INSTANCE;
        tessellator.start(kGlTriangleFan);
        tessellator.color(sunRed, sunGreen, sunBlue, (*background)[3]);
        tessellator.vertex(0.0, 100.0, 0.0);
        constexpr int segments = 16;
        tessellator.color((*background)[0], (*background)[1], (*background)[2], 0.0f);
        for (int i = 0; i <= segments; ++i) {
            const float angle =
                static_cast<float>(i) * kPi * 2.0f / static_cast<float>(segments);
            const float sinA = MathHelper::sin(angle);
            const float cosA = MathHelper::cos(angle);
            tessellator.vertex(sinA * 120.0f, cosA * 120.0f, -cosA * 40.0f * (*background)[3]);
        }
        tessellator.draw();
        gl::GL11::glPopMatrix();
        gl::GL11::glShadeModel(gl::GL11::GL_FLAT);
    }

    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE);
    gl::GL11::glPushMatrix();
    const float clearAmount =
        1.0f - client::option::rainGradient(client::option::resolve(ctx.options), ctx.world, tickDelta);
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, clearAmount);
    gl::GL11::glTranslatef(0.0f, 0.0f, 0.0f);
    gl::GL11::glRotatef(0.0f, 0.0f, 0.0f, 1.0f);
    gl::GL11::glRotatef(timeOfDay * 360.0f, 1.0f, 0.0f, 0.0f);

    float sunSize = 30.0f;
    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D,
        ctx.textureManager->getTextureId("/terrain/sun.png"));
    Tessellator& tessellator = INSTANCE;
    tessellator.startQuads();
    tessellator.vertex(-sunSize, 100.0, -sunSize, 0.0, 0.0);
    tessellator.vertex(sunSize, 100.0, -sunSize, 1.0, 0.0);
    tessellator.vertex(sunSize, 100.0, sunSize, 1.0, 1.0);
    tessellator.vertex(-sunSize, 100.0, sunSize, 0.0, 1.0);
    tessellator.draw();

    sunSize = 20.0f;
    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D,
        ctx.textureManager->getTextureId("/terrain/moon.png"));
    tessellator.startQuads();
    tessellator.vertex(-sunSize, -100.0, sunSize, 1.0, 1.0);
    tessellator.vertex(sunSize, -100.0, sunSize, 0.0, 1.0);
    tessellator.vertex(sunSize, -100.0, -sunSize, 0.0, 0.0);
    tessellator.vertex(-sunSize, -100.0, -sunSize, 1.0, 0.0);
    tessellator.draw();

    gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
    if (client::option::resolve(ctx.options).renderStars) {
        const float starBrightness =
            ctx.world->calculateSkyLightIntensity(tickDelta) * clearAmount;
        starField_.draw(starBrightness);
    }

    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    gl::GL11::glDisable(gl::GL11::GL_BLEND);
    gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
    platform::GlState::setFogEnabled(true);
    gl::GL11::glPopMatrix();

    float darkRed = red;
    float darkGreen = green;
    float darkBlue = blue;
    if (ctx.world->dimension->hasGround()) {
        darkRed = red * 0.2f + 0.04f;
        darkGreen = green * 0.2f + 0.04f;
        darkBlue = blue * 0.6f + 0.1f;
    }
    gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
    skyDome_.drawDark(darkRed, darkGreen, darkBlue);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    gl::GL11::glDepthMask(true);
    platform::GlState::endSkyPass();
}

void AtmosphereRenderer::renderClouds(const AtmosphereContext& ctx, float tickDelta)
{
    clouds_.renderClouds(ctx, tickDelta);
}

void AtmosphereRenderer::renderPrecipitation(const AtmosphereContext& ctx, float tickDelta)
{
    precipitation_.renderPrecipitation(ctx, tickDelta);
}

} // namespace net::minecraft::client::render::atmosphere
