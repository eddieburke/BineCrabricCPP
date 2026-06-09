#pragma once

#include "net/minecraft/client/render/texture/DynamicTexture.hpp"

#include <array>

namespace net::minecraft::client::render::texture {

class NetherPortalSprite : public DynamicTexture {
public:
    NetherPortalSprite();

    void tick() override;

private:
    int ticks = 0;
    std::array<std::array<std::uint8_t, 1024>, 32> frames {};
};

} // namespace net::minecraft::client::render::texture
