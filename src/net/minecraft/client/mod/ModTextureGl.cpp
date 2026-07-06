#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include <string>
#include <vector>
namespace net::minecraft::mod::detail {
struct ModTextureEntry {
  std::string path;
  int glId = -1;
};
std::vector<ModTextureEntry>& modTextureEntries();
} // namespace net::minecraft::mod::detail
namespace net::minecraft::mod {
using detail::modTextureEntries;
using detail::ModTextureEntry;
int glId(client::texture::TextureManager& tm, int textureId) {
  if(!isMod(textureId)) {
    return -1;
  }
  const int index = textureId - kModTextureBase;
  if(index < 0 || index >= static_cast<int>(modTextureEntries().size())) {
    return -1;
  }
  ModTextureEntry& entry = modTextureEntries()[static_cast<std::size_t>(index)];
  if(entry.glId < 0) {
    entry.glId = tm.getTextureId(entry.path);
  }
  return entry.glId;
}
void bind(client::texture::TextureManager& tm, int textureId) {
  const int id = glId(tm, textureId);
  if(id >= 0) {
    tm.bindTexture(id);
  }
}
} // namespace net::minecraft::mod
