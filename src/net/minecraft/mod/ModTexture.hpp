#pragma once
#include <string>
#include <vector>
#include "net/minecraft/block/Block.hpp"
namespace net::minecraft::client::texture {
class TextureManager;
}
namespace net::minecraft::mod {
inline constexpr int kModTextureBase = 256;
struct TileScale {
  int u = 0;
  int v = 0;
  double inv = 1.0 / 256.0;
};
namespace detail {
struct ModTextureEntry {
  std::string path;
  int glId = -1;
};
[[nodiscard]] std::vector<ModTextureEntry>& modTextureEntries();
} // namespace detail
[[nodiscard]] int texture(const char* path);
[[nodiscard]] bool isMod(int textureId);
[[nodiscard]] TileScale tileScale(int textureId);
[[nodiscard]] block::TerrainAtlasUv tileUv(int textureId);
[[nodiscard]] int glId(client::texture::TextureManager& tm, int textureId);
void bind(client::texture::TextureManager& tm, int textureId);
} // namespace net::minecraft::mod
