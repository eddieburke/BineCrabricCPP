#include "net/minecraft/client/resource/pack/BuiltInTexturePack.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
namespace net::minecraft::client::resource::pack {
BuiltInTexturePack::BuiltInTexturePack(const ResourcePack& resources) : resources_(resources) {
  name = "Default";
  descriptionLine1 = "The default look of Minecraft";
  const texture::RasterImage raster = texture::TextureManager::loadRasterFromBytes(resources_.readBinary("pack.png"));
  if(raster.width > 0 && raster.height > 0) {
    icon_ = raster;
  }
}
void BuiltInTexturePack::unload(texture::TextureManager& textureManager) {
  if(iconId_ >= 0) {
    textureManager.deleteTexture(iconId_);
    iconId_ = -1;
  }
}
void BuiltInTexturePack::bindIcon(texture::TextureManager& textureManager) {
  if(icon_.has_value() && iconId_ < 0) {
    iconId_ = textureManager.load(*icon_);
  }
  if(icon_.has_value() && iconId_ >= 0) {
    textureManager.bindTexture(iconId_);
    return;
  }
  gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, textureManager.getTextureId("/gui/unknown_pack.png"));
}
std::vector<std::uint8_t> BuiltInTexturePack::getResource(std::string_view path) const {
  std::string relative(path);
  if(!relative.empty() && relative.front() == '/') {
    relative.erase(relative.begin());
  }
  return resources_.readBinary(relative);
}
} // namespace net::minecraft::client::resource::pack
