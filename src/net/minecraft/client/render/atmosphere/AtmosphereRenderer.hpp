#pragma once

#include "net/minecraft/client/render/atmosphere/CloudRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/FogRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/PrecipitationRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/SkyDomeRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/StarFieldRenderer.hpp"

namespace net::minecraft::client::render::atmosphere {

struct AtmosphereContext;

class AtmosphereRenderer {
public:
    void rebuildStaticGeometry();
    void releaseGpuResources();

    void updateClearAndFogColors(const AtmosphereContext& ctx, float tickDelta);
    void applySkyFog(const AtmosphereContext& ctx);
    void applyWorldFog(const AtmosphereContext& ctx);
    void applyHandFog(const AtmosphereContext& ctx);
    void renderSky(const AtmosphereContext& ctx, float tickDelta);
    void renderClouds(const AtmosphereContext& ctx, float tickDelta);
    void renderPrecipitation(const AtmosphereContext& ctx, float tickDelta);

    void tick() { ++atmosphereTicks_; }
    [[nodiscard]] int atmosphereTicks() const { return atmosphereTicks_; }

private:
    StarFieldRenderer starField_ {};
    SkyDomeRenderer skyDome_ {};
    FogRenderer fog_ {};
    CloudRenderer clouds_ {};
    PrecipitationRenderer precipitation_ {};
    int atmosphereTicks_ = 0;
};

} // namespace net::minecraft::client::render::atmosphere
