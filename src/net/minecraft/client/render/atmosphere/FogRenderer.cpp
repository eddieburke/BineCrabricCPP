#include "net/minecraft/client/render/atmosphere/FogRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"

#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace net::minecraft::client::render::atmosphere {

namespace {

constexpr int kFogModeExp = 0x0800;
constexpr int kNvFogDistanceMode = 34138;
constexpr int kNvEyeRadial = 34139;

[[nodiscard]] bool nvFogDistanceSupported()
{
#if defined(MINECRAFT_GL_REAL)
    static const bool supported = []() {
        const char* extensions = reinterpret_cast<const char*>(::glGetString(GL_EXTENSIONS));
        return extensions != nullptr && std::strstr(extensions, "GL_NV_fog_distance") != nullptr;
    }();
    return supported;
#else
    return false;
#endif
}

} // namespace

void FogRenderer::updateClearAndFogColors(const AtmosphereContext& ctx, float tickDelta)
{
    if (ctx.world == nullptr || ctx.camera == nullptr) {
        return;
    }

    net::minecraft::World* world = ctx.world;

    const float blend = ctx.viewDistance.fogColorBlendWeight();

    const Vec3d sky = world->getSkyColor(ctx.camera, tickDelta);
    const float skyR = static_cast<float>(sky.x);
    const float skyG = static_cast<float>(sky.y);
    const float skyB = static_cast<float>(sky.z);

    const Vec3d fog = world->getFogColor(tickDelta);
    fogRed_ = static_cast<float>(fog.x);
    fogGreen_ = static_cast<float>(fog.y);
    fogBlue_ = static_cast<float>(fog.z);
    fogRed_ += (skyR - fogRed_) * blend;
    fogGreen_ += (skyG - fogGreen_) * blend;
    fogBlue_ += (skyB - fogBlue_) * blend;

    const client::option::ResolvedRenderOptions resolved = client::option::resolve(ctx.options);
    const float rain = client::option::rainGradient(resolved, world, tickDelta);
    if (rain > 0.0f) {
        const float wet = 1.0f - rain * 0.5f;
        const float wetB = 1.0f - rain * 0.4f;
        fogRed_ *= wet;
        fogGreen_ *= wet;
        fogBlue_ *= wetB;
    }

    const float thunder = client::option::thunderGradient(resolved, world, tickDelta);
    if (thunder > 0.0f) {
        const float dark = 1.0f - thunder * 0.5f;
        fogRed_ *= dark;
        fogGreen_ *= dark;
        fogBlue_ *= dark;
    }

    if (ctx.livingCamera != nullptr
        && ctx.livingCamera->isInFluid(::net::minecraft::block::material::Material::WATER)) {
        if (resolved.clearWater) {
            fogRed_ = 0.05f;
            fogGreen_ = 0.05f;
            fogBlue_ = 0.35f;
        } else {
            fogRed_ = 0.02f;
            fogGreen_ = 0.02f;
            fogBlue_ = 0.2f;
        }
    } else if (ctx.livingCamera != nullptr
        && ctx.livingCamera->isInFluid(::net::minecraft::block::material::Material::LAVA)) {
        fogRed_ = 0.6f;
        fogGreen_ = 0.1f;
        fogBlue_ = 0.0f;
    } else if (resolved.customFogColor) {
        fogRed_ = resolved.fogColorRed;
        fogGreen_ = resolved.fogColorGreen;
        fogBlue_ = resolved.fogColorBlue;
    }

    gl::GL11::glClearColor(fogRed_, fogGreen_, fogBlue_, 0.0f);
}

void FogRenderer::applySkyFog(const AtmosphereContext& ctx)
{
    if (ctx.camera == nullptr) {
        return;
    }

    pushFogColor(ctx);
    switch (cameraMedium(ctx)) {
    case CameraMedium::Water:
        pushWaterFog(ctx);
        break;
    case CameraMedium::Lava:
        pushLavaFog();
        break;
    case CameraMedium::Air:
        if (client::option::resolve(ctx.options).customFog) {
            pushCustomFog(ctx, FogDistanceProfile::Sky);
        } else {
            pushVanillaLinearFog(ctx, FogDistanceProfile::Sky);
        }
        break;
    }
}

void FogRenderer::applyWorldFog(const AtmosphereContext& ctx)
{
    if (ctx.camera == nullptr) {
        return;
    }

    pushFogColor(ctx);
    switch (cameraMedium(ctx)) {
    case CameraMedium::Water:
        pushWaterFog(ctx);
        break;
    case CameraMedium::Lava:
        pushLavaFog();
        break;
    case CameraMedium::Air:
        if (client::option::resolve(ctx.options).customFog) {
            pushCustomFog(ctx, FogDistanceProfile::World);
        } else {
            pushVanillaLinearFog(ctx, FogDistanceProfile::World);
        }
        break;
    }
}

void FogRenderer::applyHandFog(const AtmosphereContext& ctx)
{
    applyWorldFog(ctx);
}

FogRenderer::CameraMedium FogRenderer::cameraMedium(const AtmosphereContext& ctx) const
{
    if (ctx.livingCamera == nullptr) {
        return CameraMedium::Air;
    }
    if (ctx.livingCamera->isInFluid(::net::minecraft::block::material::Material::WATER)) {
        return CameraMedium::Water;
    }
    if (ctx.livingCamera->isInFluid(::net::minecraft::block::material::Material::LAVA)) {
        return CameraMedium::Lava;
    }
    return CameraMedium::Air;
}

void FogRenderer::pushFogColor(const AtmosphereContext& ctx)
{
    (void)ctx;
    gl::GL11::glFogfv(gl::GL11::GL_FOG_COLOR, fogColorBuffer(fogRed_, fogGreen_, fogBlue_, 1.0f));
}

void FogRenderer::pushWaterFog(const AtmosphereContext& ctx)
{
    gl::GL11::glFogi(gl::GL11::GL_FOG_MODE, kFogModeExp);
    gl::GL11::glFogf(gl::GL11::GL_FOG_DENSITY,
        client::option::resolve(ctx.options).clearWater ? 0.02f : 0.1f);
}

void FogRenderer::pushLavaFog()
{
    gl::GL11::glFogi(gl::GL11::GL_FOG_MODE, kFogModeExp);
    gl::GL11::glFogf(gl::GL11::GL_FOG_DENSITY, 2.0f);
}

void FogRenderer::pushCustomFog(const AtmosphereContext& ctx, FogDistanceProfile distanceProfile)
{
    const client::option::ResolvedRenderOptions resolved = client::option::resolve(ctx.options);
    if (resolved.customFogLinear) {
        applyLinearDistances(ctx, resolved.fogStart, resolved.fogEnd, distanceProfile);
    } else {
        gl::GL11::glFogi(gl::GL11::GL_FOG_MODE, kFogModeExp);
        gl::GL11::glFogf(gl::GL11::GL_FOG_DENSITY,
            resolved.fogDensity / std::max(1.0f, ctx.viewDistance.renderScale()));
    }
    pushNvFogDistance(ctx);
    pushNetherStartOverride(ctx);
}

void FogRenderer::pushVanillaLinearFog(const AtmosphereContext& ctx, FogDistanceProfile distanceProfile)
{
    applyLinearDistances(ctx, 0.25f, 1.0f, distanceProfile);
    pushNvFogDistance(ctx);
    pushNetherStartOverride(ctx);
}

void FogRenderer::applyLinearDistances(const AtmosphereContext& ctx, float startFactor, float endFactor,
    FogDistanceProfile distanceProfile)
{
    gl::GL11::glFogi(gl::GL11::GL_FOG_MODE, gl::GL11::GL_LINEAR);
    if (distanceProfile == FogDistanceProfile::Sky) {
        gl::GL11::glFogf(gl::GL11::GL_FOG_START, 0.0f);
        gl::GL11::glFogf(gl::GL11::GL_FOG_END, ctx.viewDistance.skyFogEndBlocks());
    } else {
        gl::GL11::glFogf(gl::GL11::GL_FOG_START, ctx.viewDistance.worldFogStartBlocks(startFactor));
        gl::GL11::glFogf(gl::GL11::GL_FOG_END, ctx.viewDistance.worldFogEndBlocks(endFactor));
    }
}

void FogRenderer::pushNetherStartOverride(const AtmosphereContext& ctx)
{
    if (ctx.world != nullptr && ctx.world->dimension != nullptr && ctx.world->dimension->isNether) {
        gl::GL11::glFogf(gl::GL11::GL_FOG_START, 0.0f);
    }
}

void FogRenderer::pushNvFogDistance(const AtmosphereContext& ctx)
{
    if (!nvFogDistanceSupported()) {
        return;
    }
    const client::option::ResolvedRenderOptions resolved = client::option::resolve(ctx.options);
    if (!resolved.customFog || resolved.sphericalFog) {
        gl::GL11::glFogi(kNvFogDistanceMode, kNvEyeRadial);
    }
}

const float* FogRenderer::fogColorBuffer(float red, float green, float blue, float alpha)
{
    fogColorBuffer_[0] = red;
    fogColorBuffer_[1] = green;
    fogColorBuffer_[2] = blue;
    fogColorBuffer_[3] = alpha;
    return fogColorBuffer_.data();
}

} // namespace net::minecraft::client::render::atmosphere
