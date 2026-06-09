#pragma once

// Faithful 1:1 port of net.minecraft.client.render.texture.DynamicTexture (beta 1.7.3).

#include <array>
#include <cstdint>

namespace net::minecraft::client::texture {
class TextureManager;
}

namespace net::minecraft::client::render::texture {

class DynamicTexture {
public:
    explicit DynamicTexture(int sprite)
        : sprite(sprite)
    {
    }

    virtual ~DynamicTexture() = default;

    virtual void tick()
    {
    }

    virtual void bind(::net::minecraft::client::texture::TextureManager& textureManager);

    std::array<std::uint8_t, 1024> pixels {};
    int sprite = 0;
    int copyTo = 0;
    int replicate = 1;
    int atlas = 0;
};

} // namespace net::minecraft::client::render::texture
