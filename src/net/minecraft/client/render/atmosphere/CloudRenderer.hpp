#pragma once

namespace net::minecraft::client::render::atmosphere {

struct AtmosphereContext;

class CloudRenderer {
public:
    void renderClouds(const AtmosphereContext& ctx, float tickDelta);

private:
    void renderFancyClouds(const AtmosphereContext& ctx, float tickDelta);
};

} // namespace net::minecraft::client::render::atmosphere
