#pragma once
#include "net/minecraft/client/render/texture/LiquidSprite.hpp"

namespace net::minecraft::client::render::texture {
class WaterSprite : public LiquidSprite {
   public:
    WaterSprite();
    void tick() override;
};
}  // namespace net::minecraft::client::render::texture
