#pragma once
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::texture {
class SkinImageProcessor {
 public:
 [[nodiscard]] RasterImage process(const RasterImage& image);
 [[nodiscard]] static bool detectSlimArms(const RasterImage& image);
};
} // namespace net::minecraft::client::texture
