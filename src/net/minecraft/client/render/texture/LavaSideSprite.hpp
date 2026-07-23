#pragma once
#include "net/minecraft/client/render/texture/LiquidSprite.hpp"
namespace net::minecraft::client::render::texture {
class LavaSideSprite : public LiquidSprite {
 public:
 LavaSideSprite();
 void tick() override;
};
} // namespace net::minecraft::client::render::texture
