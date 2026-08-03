#include "net/minecraft/client/render/PbrTextures.hpp"
#include <string>
#include <vector>
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
namespace net::minecraft::client::render {
namespace {
std::uint32_t packArgb(int alpha, int red, int green, int blue) {
 return (static_cast<std::uint32_t>(alpha) << 24) | (static_cast<std::uint32_t>(red) << 16) |
        (static_cast<std::uint32_t>(green) << 8) | static_cast<std::uint32_t>(blue);
}
std::uint32_t blendChannels4(int c0, int c1, int c2, int c3, float w0, float w1, float w2,
                             float w3) {
 const auto channel = [c0, c1, c2, c3, w0, w1, w2, w3](int shift) {
  const int v0 = static_cast<int>((static_cast<std::uint32_t>(c0) >> shift) & 0xFF);
  const int v1 = static_cast<int>((static_cast<std::uint32_t>(c1) >> shift) & 0xFF);
  const int v2 = static_cast<int>((static_cast<std::uint32_t>(c2) >> shift) & 0xFF);
  const int v3 = static_cast<int>((static_cast<std::uint32_t>(c3) >> shift) & 0xFF);
  return static_cast<int>(v0 * w0 + v1 * w1 + v2 * w2 + v3 * w3 + 0.5f);
 };
 return packArgb(channel(24), channel(16), channel(8), channel(0));
}
} // namespace
void PbrTextures::pbrDownsampleMip(const std::uint8_t* src, int srcWidth,
                                   std::uint8_t* dst, int dstWidth, int dstHeight,
                                   Type type, bool labPbr) noexcept {
 for(int y = 0; y < dstHeight; ++y) {
  for(int x = 0; x < dstWidth; ++x) {
   const auto pixelAt = [src, srcWidth](int px, int py) {
    const std::size_t offset = (static_cast<std::size_t>(py) * srcWidth + px) * 4;
    return (static_cast<std::uint32_t>(src[offset + 0]) << 16) |
           (static_cast<std::uint32_t>(src[offset + 1]) << 8) |
           static_cast<std::uint32_t>(src[offset + 2]) |
           (static_cast<std::uint32_t>(src[offset + 3]) << 24);
   };
   const std::uint32_t blended =
       pbrBlendMip4(pixelAt(x * 2, y * 2), pixelAt(x * 2 + 1, y * 2), pixelAt(x * 2, y * 2 + 1),
                    pixelAt(x * 2 + 1, y * 2 + 1), type, labPbr);
   const std::size_t offset = (static_cast<std::size_t>(y) * dstWidth + x) * 4;
   dst[offset + 0] = static_cast<std::uint8_t>((blended >> 16) & 0xFF);
   dst[offset + 1] = static_cast<std::uint8_t>((blended >> 8) & 0xFF);
   dst[offset + 2] = static_cast<std::uint8_t>(blended & 0xFF);
   dst[offset + 3] = static_cast<std::uint8_t>((blended >> 24) & 0xFF);
  }
 }
}
net::minecraft::client::texture::RasterImage PbrTextures::pbrScaleNearest(const net::minecraft::client::texture::RasterImage& source,
                                                  int newWidth, int newHeight) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pbr/util/ImageManipulationUtil.java
 net::minecraft::client::texture::RasterImage out;
 if(source.width <= 0 || source.height <= 0 || newWidth <= 0 || newHeight <= 0) {
  return out;
 }
 out.width = newWidth;
 out.height = newHeight;
 out.argb.resize(static_cast<std::size_t>(newWidth) * newHeight);
 const float xScale = static_cast<float>(newWidth) / static_cast<float>(source.width);
 const float yScale = static_cast<float>(newHeight) / static_cast<float>(source.height);
 for(int y = 0; y < newHeight; ++y) {
  for(int x = 0; x < newWidth; ++x) {
   const int srcX = static_cast<int>((x + 0.5f) / xScale);
   const int srcY = static_cast<int>((y + 0.5f) / yScale);
   out.argb[static_cast<std::size_t>(y) * newWidth + x] =
       source.argb[static_cast<std::size_t>(srcY) * source.width + srcX];
  }
 }
 return out;
}
net::minecraft::client::texture::RasterImage PbrTextures::pbrScaleBilinear(const net::minecraft::client::texture::RasterImage& source,
                                                   int newWidth, int newHeight) {
 net::minecraft::client::texture::RasterImage out;
 if(source.width <= 0 || source.height <= 0 || newWidth <= 0 || newHeight <= 0) {
  return out;
 }
 out.width = newWidth;
 out.height = newHeight;
 out.argb.resize(static_cast<std::size_t>(newWidth) * newHeight);
 const float xScale = static_cast<float>(newWidth) / static_cast<float>(source.width);
 const float yScale = static_cast<float>(newHeight) / static_cast<float>(source.height);
 for(int y = 0; y < newHeight; ++y) {
  for(int x = 0; x < newWidth; ++x) {
   const float unscaledX = (x + 0.5f) / xScale;
   const float unscaledY = (y + 0.5f) / yScale;
   int x1 = static_cast<int>(unscaledX + 0.5f);
   int y1 = static_cast<int>(unscaledY + 0.5f);
   int x0 = x1 - 1;
   int y0 = y1 - 1;
   const bool x0Valid = x0 >= 0;
   const bool y0Valid = y0 >= 0;
   const bool x1Valid = x1 < source.width;
   const bool y1Valid = y1 < source.height;
   std::uint32_t color = 0;
   if(x0Valid && y0Valid && x1Valid && y1Valid) {
    const float leftWeight = (x1 + 0.5f) - unscaledX;
    const float rightWeight = unscaledX - (x0 + 0.5f);
    const float topWeight = (y1 + 0.5f) - unscaledY;
    const float bottomWeight = unscaledY - (y0 + 0.5f);
    color = blendChannels4(source.argb[static_cast<std::size_t>(y0) * source.width + x0],
                           source.argb[static_cast<std::size_t>(y0) * source.width + x1],
                           source.argb[static_cast<std::size_t>(y1) * source.width + x0],
                           source.argb[static_cast<std::size_t>(y1) * source.width + x1],
                           leftWeight * topWeight, rightWeight * topWeight, leftWeight * bottomWeight,
                           rightWeight * bottomWeight);
   } else if(x0Valid && x1Valid) {
    const float leftWeight = (x1 + 0.5f) - unscaledX;
    const float rightWeight = unscaledX - (x0 + 0.5f);
    const int validY = y0Valid ? y0 : y1;
    const int left = static_cast<int>(source.argb[static_cast<std::size_t>(validY) * source.width + x0]);
    const int right = static_cast<int>(source.argb[static_cast<std::size_t>(validY) * source.width + x1]);
    color = blendChannels4(left, right, left, right, leftWeight, rightWeight, 0.0f, 0.0f);
   } else if(y0Valid && y1Valid) {
    const float topWeight = (y1 + 0.5f) - unscaledY;
    const float bottomWeight = unscaledY - (y0 + 0.5f);
    const int validX = x0Valid ? x0 : x1;
    const int top = static_cast<int>(source.argb[static_cast<std::size_t>(y0) * source.width + validX]);
    const int bottom = static_cast<int>(source.argb[static_cast<std::size_t>(y1) * source.width + validX]);
    color = blendChannels4(top, bottom, top, bottom, topWeight, bottomWeight, 0.0f, 0.0f);
   } else {
    color = source.argb[static_cast<std::size_t>(y0Valid ? y0 : y1) * source.width + (x0Valid ? x0 : x1)];
   }
   out.argb[static_cast<std::size_t>(y) * newWidth + x] = color;
  }
 }
 return out;
}
std::unordered_map<int, PbrTextures::CacheEntry>& PbrTextures::cache() noexcept {
 static std::unordered_map<int, CacheEntry> entries;
 return entries;
}
void PbrTextures::clear() noexcept {
 cache().clear();
}
void PbrTextures::onNewFrame() noexcept {
 // Loading is synchronous; the Java onNewFrame only processes loads deferred
 // by getOrLoadHolder, which this port never defers.
}
void PbrTextures::onDeleteTexture(int textureId) noexcept {
 if(textureId > 0) {
  cache().erase(textureId);
 } else {
  cache().clear();
 }
}
void PbrTextures::uploadMipLevels(int textureId, const net::minecraft::client::texture::RasterImage& image, Type type,
                                  bool labPbr) {
 std::vector<std::uint8_t> rgba = net::minecraft::client::texture::TextureManager::rasterToRgba(image);
 core::TextureBindScope bindScope(textureId);
 // PBRAtlasTexture.upload(): clamp-to-edge + NEAREST; minification samples the
 // LabPBR-generated mips with NEAREST_MIPMAP_NEAREST (IrisSamplers terrain
 // cache / MIPPED_NEAREST_NEAREST with anisotropy off).
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Nearest);
 int maxLevel = 0;
 if(net::minecraft::client::texture::TextureManager::MIPMAP) {
  int mipWidth = image.width;
  int mipHeight = image.height;
  while(mipWidth > 1 && mipHeight > 1 && maxLevel < 4) {
   mipWidth >>= 1;
   mipHeight >>= 1;
   ++maxLevel;
  }
 }
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::MinFilter,
                   maxLevel > 0 ? gl::filter::NearestMipmapNearest : gl::filter::Nearest);
 ::glTexParameteri(gl::cap::Texture2D, gl::tex::MaxLevel, maxLevel);
 ::glTexImage2D(gl::cap::Texture2D, 0, gl::pixel::Rgba8, image.width, image.height, 0,
                gl::pixel::Rgba, gl::pixel::UnsignedByte, rgba.data());
 for(int level = 1; level <= maxLevel; ++level) {
  const int sourceWidth = image.width >> (level - 1);
  const int targetWidth = image.width >> level;
  const int targetHeight = image.height >> level;
  pbrDownsampleMip(rgba.data(), sourceWidth, rgba.data(), targetWidth,
                   targetHeight, type, labPbr);
  ::glTexImage2D(gl::cap::Texture2D, level, gl::pixel::Rgba8, targetWidth, targetHeight, 0,
                 gl::pixel::Rgba, gl::pixel::UnsignedByte, rgba.data());
 }
}
int PbrTextures::resolveCompanion(int diffuseTextureId, Type type,
                                  net::minecraft::client::texture::TextureManager& manager, bool labPbr) {
 const std::string path = manager.getCompanionTexturePath(diffuseTextureId, suffix(type));
 if(path.empty()) {
  return -1;
 }
 if(!manager.resourceExists(path)) {
  return -1;
 }
 net::minecraft::client::texture::RasterImage image = manager.loadRasterForResource(path);
 if(image.width <= 0 || image.height <= 0) {
  return -1;
 }
 if(!registry::TextureRegistry::isCustomTexture(diffuseTextureId)) {
  // Grid atlases must line up with the diffuse atlas UV grid; scale the
  // companion when the pack ships a different resolution (nearest for
  // integer multiples, bilinear otherwise — AtlasPBRLoader sprite scaling).
  int atlasWidth = 0;
  int atlasHeight = 0;
  if(manager.getTextureDimensionsForId(diffuseTextureId, atlasWidth, atlasHeight) &&
     (image.width != atlasWidth || image.height != atlasHeight)) {
   image = (atlasWidth % image.width == 0 && atlasHeight % image.height == 0)
               ? pbrScaleNearest(image, atlasWidth, atlasHeight)
               : pbrScaleBilinear(image, atlasWidth, atlasHeight);
   if(image.width <= 0 || image.height <= 0) {
    return -1;
   }
  }
 }
 const int textureId = manager.getTextureId(path);
 if(textureId <= 0 || manager.isMissingTextureId(textureId)) {
  return -1;
 }
 int existingWidth = 0;
 int existingHeight = 0;
 if(!manager.getTextureDimensionsForId(textureId, existingWidth, existingHeight) ||
    existingWidth != image.width || existingHeight != image.height) {
  return -1;
 }
 uploadMipLevels(textureId, image, type, labPbr);
 return textureId;
}
PbrTextures::Holder PbrTextures::getOrLoad(int diffuseTextureId, net::minecraft::client::texture::TextureManager& manager,
                                           bool labPbr) {
 if(diffuseTextureId <= 0) {
  return {};
 }
 auto& entries = cache();
 if(const auto found = entries.find(diffuseTextureId); found != entries.end()) {
  if(found->second.labPbr == labPbr) {
   return found->second.holder;
  }
  // Texture format changed (Java TextureFormatLoader.onFormatChange →
  // PBRTextureManager.clear) — re-resolve with the new blend rules.
  entries.erase(found);
 }
 CacheEntry entry;
 entry.labPbr = labPbr;
 entry.holder.normal = resolveCompanion(diffuseTextureId, Type::Normal, manager, labPbr);
 entry.holder.specular = resolveCompanion(diffuseTextureId, Type::Specular, manager, labPbr);
 entries[diffuseTextureId] = entry;
 return entry.holder;
}
} // namespace net::minecraft::client::render
