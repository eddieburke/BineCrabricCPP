#pragma once

#include "net/minecraft/client/render/texture/LiquidSprite.hpp"

namespace net::minecraft::client::render::texture {

class LavaSprite : public LiquidSprite {
public:
    LavaSprite();

    void tick() override;
};

} // namespace net::minecraft::client::render::texture
