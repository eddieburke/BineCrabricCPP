#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
namespace net::minecraft::client::render {
enum class AtlasDomain {
 Terrain,
 Items
};
struct ResolvedTexture {
 double uMin = 0.0;
 double uMax = 0.0;
 double vMin = 0.0;
 double vMax = 0.0;
 double uScale = 1.0 / 256.0;
 double vScale = 1.0 / 256.0;
 int glId = -1;
};
[[nodiscard]] inline const char* atlasPathFor(AtlasDomain domain) noexcept {
 return domain == AtlasDomain::Terrain ? "/terrain.png" : "/gui/items.png";
}
[[nodiscard]] inline ResolvedTexture resolveBlockTextureUv(int textureId) {
 if(net::minecraft::registry::TextureRegistry::isCustomTexture(textureId)) {
  const net::minecraft::registry::TextureRegistry::Entry* entry =
      net::minecraft::registry::TextureRegistry::getEntry(textureId);
  const double width = (entry != nullptr && entry->width > 0) ? static_cast<double>(entry->width) : 16.0;
  const double height = (entry != nullptr && entry->height > 0) ? static_cast<double>(entry->height) : 16.0;
  return {0.0, 1.0, 0.0, 1.0, 1.0 / width, 1.0 / height, -1};
 }
 const int u = net::minecraft::block::Block::textureAtlasU(textureId);
 const int v = net::minecraft::block::Block::textureAtlasV(textureId);
 constexpr double inv = 1.0 / 256.0;
 return {static_cast<double>(u) * inv,
         static_cast<double>(u + 16) * inv,
         static_cast<double>(v) * inv,
         static_cast<double>(v + 16) * inv,
         inv,
         inv,
         -1};
}
[[nodiscard]] inline ResolvedTexture resolveBlockTexture(
    int textureId, net::minecraft::client::texture::TextureManager& textureManager, AtlasDomain domain) {
 ResolvedTexture resolved = resolveBlockTextureUv(textureId);
 if(net::minecraft::registry::TextureRegistry::isCustomTexture(textureId)) {
  resolved.glId = net::minecraft::registry::TextureRegistry::resolveGlId(textureId, textureManager);
 } else {
  resolved.glId = textureManager.getTextureId(atlasPathFor(domain));
 }
 return resolved;
}
} // namespace net::minecraft::client::render
