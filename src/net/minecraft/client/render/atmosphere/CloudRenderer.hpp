#pragma once
namespace net::minecraft::client::render::atmosphere {
struct AtmosphereContext;
void renderClouds(const AtmosphereContext& ctx, float tickDelta);
} // namespace net::minecraft::client::render::atmosphere
