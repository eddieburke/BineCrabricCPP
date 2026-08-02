#include "net/minecraft/client/render/shaderpack/PackTexture.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::render {
DecodedTexture decodeTexture(const std::string& bytes) {
 DecodedTexture output;
 const net::minecraft::client::texture::RasterImage image =
     net::minecraft::client::texture::TextureManager::loadRasterFromBytes(
         std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
 if(image.width <= 0 || image.height <= 0) {
  return output;
 }
 output.width = image.width;
 output.height = image.height;
 output.rgba = net::minecraft::client::texture::TextureManager::rasterToRgba(image);
 return output;
}
} // namespace net::minecraft::client::render
