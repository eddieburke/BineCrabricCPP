#pragma once

#include "net/minecraft/client/render/texture/DynamicTexture.hpp"

#include <array>

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::render::texture {

class CompassSprite : public DynamicTexture {
public:
    explicit CompassSprite(Minecraft& minecraft);

    void tick() override;

private:
    Minecraft& minecraft_;
    std::array<std::uint32_t, 256> compass {};
    double angle = 0.0;
    double angleDelta = 0.0;
};

} // namespace net::minecraft::client::render::texture
