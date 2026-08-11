#pragma once
namespace net::minecraft::client::render::atmosphere {
struct AtmosphereContext;
void renderSkyDome(const AtmosphereContext& ctx, float tickDelta);
void renderSkyStars(const AtmosphereContext& ctx, float tickDelta, float starBrightness);
void renderSkyVoid(const AtmosphereContext& ctx, float tickDelta);
} // namespace net::minecraft::client::render::atmosphere
