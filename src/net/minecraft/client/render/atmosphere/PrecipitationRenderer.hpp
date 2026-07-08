#pragma once
#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft::client::render::atmosphere {
struct AtmosphereContext;

class PrecipitationRenderer {
   public:
    void renderPrecipitation(const AtmosphereContext& ctx, float tickDelta);

   private:
    JavaRandom random_{};
};
}  // namespace net::minecraft::client::render::atmosphere
