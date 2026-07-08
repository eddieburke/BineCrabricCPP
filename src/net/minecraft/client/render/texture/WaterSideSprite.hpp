#pragma once
#include "net/minecraft/client/render/texture/LiquidSprite.hpp"

namespace net::minecraft::client::render::texture {
class WaterSideSprite : public LiquidSprite {
   public:
    WaterSideSprite();
    void tick() override;
};
}  // namespace net::minecraft::client::render::texture
