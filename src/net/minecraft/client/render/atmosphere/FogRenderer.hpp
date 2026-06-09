#pragma once

#include <array>

namespace net::minecraft::client::render::atmosphere {

struct AtmosphereContext;

enum class FogDistanceProfile {
    Sky,
    World,
};

class FogRenderer {
public:
    void updateClearAndFogColors(const AtmosphereContext& ctx, float tickDelta);

    void applySkyFog(const AtmosphereContext& ctx);
    void applyWorldFog(const AtmosphereContext& ctx);
    void applyHandFog(const AtmosphereContext& ctx);

    [[nodiscard]] float fogRed() const { return fogRed_; }
    [[nodiscard]] float fogGreen() const { return fogGreen_; }
    [[nodiscard]] float fogBlue() const { return fogBlue_; }

private:
    enum class CameraMedium { Air, Water, Lava };

    [[nodiscard]] CameraMedium cameraMedium(const AtmosphereContext& ctx) const;
    void pushFogColor(const AtmosphereContext& ctx);
    void pushWaterFog(const AtmosphereContext& ctx);
    void pushLavaFog();
    void pushCustomFog(const AtmosphereContext& ctx, FogDistanceProfile distanceProfile);
    void pushVanillaLinearFog(const AtmosphereContext& ctx, FogDistanceProfile distanceProfile);
    void applyLinearDistances(const AtmosphereContext& ctx, float startFactor, float endFactor,
        FogDistanceProfile distanceProfile);
    void pushNetherStartOverride(const AtmosphereContext& ctx);
    void pushNvFogDistance(const AtmosphereContext& ctx);

    const float* fogColorBuffer(float red, float green, float blue, float alpha);

    float fogRed_ = 0.0f;
    float fogGreen_ = 0.0f;
    float fogBlue_ = 0.0f;
    std::array<float, 16> fogColorBuffer_ {};
};

} // namespace net::minecraft::client::render::atmosphere
