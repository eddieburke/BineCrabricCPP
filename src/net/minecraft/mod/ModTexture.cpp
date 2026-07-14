#include "net/minecraft/mod/ModTexture.hpp"
#include <string>
#include <vector>
namespace net::minecraft::mod::detail {
std::vector<ModTextureEntry>& modTextureEntries() {
  static std::vector<ModTextureEntry> table;
  return table;
}
} // namespace net::minecraft::mod::detail
namespace net::minecraft::mod {
namespace {
using detail::modTextureEntries;
using detail::ModTextureEntry;
std::string normalizePath(const char* path) {
  if(path == nullptr) {
    return {};
  }
  std::string out = path;
  while(!out.empty() && (out.front() == '/' || out.front() == '\\')) {
    out.erase(out.begin());
  }
  return out;
}
block::TerrainAtlasUv uvFromBounds(const TileScale& tile, const net::minecraft::Box& bounds) {
  double uMin = (static_cast<double>(tile.u) + bounds.minX * 16.0) * tile.inv;
  double uMax = (static_cast<double>(tile.u) + bounds.maxX * 16.0 - 0.01) * tile.inv;
  double vMin = (static_cast<double>(tile.v) + bounds.minZ * 16.0) * tile.inv;
  double vMax = (static_cast<double>(tile.v) + bounds.maxZ * 16.0 - 0.01) * tile.inv;
  if(bounds.minX < 0.0 || bounds.maxX > 1.0) {
    uMin = static_cast<double>(tile.u) * tile.inv;
    uMax = (static_cast<double>(tile.u) + 15.99) * tile.inv;
  }
  if(bounds.minZ < 0.0 || bounds.maxZ > 1.0) {
    vMin = static_cast<double>(tile.v) * tile.inv;
    vMax = (static_cast<double>(tile.v) + 15.99) * tile.inv;
  }
  return {uMin, uMax, vMin, vMax};
}
block::TerrainAtlasUv tileUvBounded(int textureId, const net::minecraft::Box& bounds) {
  if(textureId < kModTextureBase) {
    return block::Block::terrainTileUv(textureId);
  }
  const TileScale tile{0, 0, 1.0 / 16.0};
  return uvFromBounds(tile, bounds);
}
} // namespace
int texture(const char* path) {
  const std::string normalized = normalizePath(path);
  if(normalized.empty()) {
    return 0;
  }
  for(std::size_t i = 0; i < modTextureEntries().size(); ++i) {
    if(modTextureEntries()[i].path == normalized) {
      return kModTextureBase + static_cast<int>(i);
    }
  }
  modTextureEntries().push_back({normalized, -1});
  return kModTextureBase + static_cast<int>(modTextureEntries().size()) - 1;
}
bool isMod(int textureId) {
  return textureId >= kModTextureBase;
}
TileScale tileScale(int textureId) {
  if(isMod(textureId)) {
    return {0, 0, 1.0 / 16.0};
  }
  return {block::Block::textureAtlasU(textureId), block::Block::textureAtlasV(textureId), 1.0 / 256.0};
}
block::TerrainAtlasUv tileUv(int textureId) {
  return tileUvBounded(textureId, net::minecraft::Box{0.0, 0.0, 0.0, 1.0, 1.0, 1.0});
}
__attribute__((weak)) int glId(client::texture::TextureManager& /*tm*/, int /*textureId*/) {
  return -1;
}
__attribute__((weak)) void bind(client::texture::TextureManager& /*tm*/, int /*textureId*/) {
}
} // namespace net::minecraft::mod
