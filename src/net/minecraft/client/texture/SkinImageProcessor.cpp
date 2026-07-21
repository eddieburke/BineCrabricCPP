#include "net/minecraft/client/texture/SkinImageProcessor.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
namespace net::minecraft::client::texture {
namespace {
[[nodiscard]] std::uint32_t samplePixel(const RasterImage& image, int x, int y) {
 if(x < 0 || y < 0 || x >= image.width || y >= image.height) {
  return 0;
 }
 return image.argb[static_cast<std::size_t>(x + y * image.width)];
}
[[nodiscard]] bool isTransparent(std::uint32_t pixel) {
 return ((pixel >> 24U) & 0xFFU) < 128U;
}
void applySkinAlphaRegions(std::uint32_t* data, int width, int height) {
 auto setTransparent = [&](int x1, int y1, int x2, int y2) {
  auto hasTransparency = [&]() {
   for(int x = x1; x < x2; ++x) {
    for(int y = y1; y < y2; ++y) {
     if(isTransparent(data[static_cast<std::size_t>(x + y * width)])) {
      return true;
     }
    }
   }
   return false;
  };
  if(hasTransparency()) {
   return;
  }
  for(int x = x1; x < x2; ++x) {
   for(int y = y1; y < y2; ++y) {
    const std::size_t index = static_cast<std::size_t>(x + y * width);
    data[index] &= 0x00FFFFFFU;
   }
  }
 };
 auto setOpaque = [&](int x1, int y1, int x2, int y2) {
  for(int x = x1; x < x2; ++x) {
   for(int y = y1; y < y2; ++y) {
    const std::size_t index = static_cast<std::size_t>(x + y * width);
    data[index] |= 0xFF000000U;
   }
  }
 };
 setOpaque(0, 0, 32, 16);
 setTransparent(32, 0, 64, 32);
 setOpaque(0, 16, 64, 32);
 if(height < 64) {
  return;
 }
 setOpaque(32, 32, 64, 48);
 setTransparent(0, 32, 32, 64);
 setOpaque(0, 48, 64, 64);
}
} // namespace
bool SkinImageProcessor::detectSlimArms(const RasterImage& image) {
 if(image.width != 64 || image.height < 64) {
  return false;
 }
 return isTransparent(samplePixel(image, 54, 20));
}
RasterImage SkinImageProcessor::process(const RasterImage& image) {
 if(image.width <= 0 || image.height <= 0) {
  return {};
 }
 RasterImage result;
 result.width = 64;
 result.height = 32;
 result.argb.assign(static_cast<std::size_t>(result.width * result.height), 0);
 const int copyWidth = std::min(image.width, result.width);
 const int copyHeight = std::min(image.height, result.height);
 for(int y = 0; y < copyHeight; ++y) {
  for(int x = 0; x < copyWidth; ++x) {
   result.argb[static_cast<std::size_t>(x + y * result.width)] =
       image.argb[static_cast<std::size_t>(x + y * image.width)];
  }
 }
 applySkinAlphaRegions(result.argb.data(), result.width, result.height);
 return result;
}
} // namespace net::minecraft::client::texture
