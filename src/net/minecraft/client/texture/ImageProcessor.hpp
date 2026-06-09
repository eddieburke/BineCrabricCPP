#pragma once

#include "net/minecraft/client/texture/TextureManager.hpp"

namespace net::minecraft::client::texture {

class ImageProcessor {
public:
    ImageProcessor() = default;
    virtual ~ImageProcessor() = default;

    [[nodiscard]] virtual RasterImage process(const RasterImage& image) = 0;
};

} // namespace net::minecraft::client::texture
