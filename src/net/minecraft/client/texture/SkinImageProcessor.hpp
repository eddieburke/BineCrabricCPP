#pragma once
#include "net/minecraft/client/texture/ImageProcessor.hpp"
namespace net::minecraft::client::texture {
class SkinImageProcessor : public ImageProcessor {
 public:
 [[nodiscard]] RasterImage process(const RasterImage& image) override;
 [[nodiscard]] static bool detectSlimArms(const RasterImage& image);
};
} // namespace net::minecraft::client::texture
