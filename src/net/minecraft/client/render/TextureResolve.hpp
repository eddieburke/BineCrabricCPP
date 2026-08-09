#pragma once
#include <algorithm>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
namespace net::minecraft::client::render {
inline constexpr double kTextureEdgeInset = net::minecraft::block::TerrainAtlasUv::EDGE_INSET;
enum class AtlasDomain {
 Terrain,
 Items
};
// Where a texture id's pixels live, and how UV space maps onto them.
//
// There are exactly two cases, and this is the only place that distinguishes
// them:
//
//   vanilla - the id is one 16x16 tile of a 256x256 atlas. uMin..vMax is that
//             tile's sub-rect and one texel is 1/256 of UV space.
//   mod     - the id is a standalone image owned by TextureRegistry. It fills
//             UV space (0..1), and one "pixel step" is 1/16 regardless of the
//             image's real pixel size.
//
// The mod scale is deliberately 1/16 and not 1/width: callers phrase offsets in
// 16ths of a tile ((u + n * 16) * uScale), and the JSON model baker already
// bakes face uvs against the model's declared texture_size, a UV resolution
// decoupled from the image's real size. Deriving the scale from the image width
// instead would sample only the top-left 16 pixels of a larger mod texture.
struct ResolvedTexture {
 double uMin = 0.0;
 double uMax = 0.0;
 double vMin = 0.0;
 double vMax = 0.0;
 double uScale = 1.0 / 256.0;
 double vScale = 1.0 / 256.0;
 int glId = -1;
 // True when the id is a mod texture: its own image, bound on its own glId,
 // rather than a tile of the shared atlas. Callers branch on this instead of
 // re-querying TextureRegistry.
 bool isModTexture = false;
 [[nodiscard]] double safeUMin() const noexcept {
  return uMin + kTextureEdgeInset * uScale;
 }
 [[nodiscard]] double safeUMax() const noexcept {
  return uMax - kTextureEdgeInset * uScale;
 }
 [[nodiscard]] double safeVMin() const noexcept {
  return vMin + kTextureEdgeInset * vScale;
 }
 [[nodiscard]] double safeVMax() const noexcept {
  return vMax - kTextureEdgeInset * vScale;
 }
 [[nodiscard]] double clampU(double value) const noexcept {
  return std::clamp(value, safeUMin(), safeUMax());
 }
 [[nodiscard]] double clampV(double value) const noexcept {
  return std::clamp(value, safeVMin(), safeVMax());
 }
 [[nodiscard]] double uFromStart(double blockCoordinate) const noexcept {
  return clampU(uMin + blockCoordinate * 16.0 * uScale);
 }
 [[nodiscard]] double uFromEnd(double blockCoordinate) const noexcept {
  return clampU(uMax - blockCoordinate * 16.0 * uScale);
 }
 [[nodiscard]] double vFromStart(double blockCoordinate) const noexcept {
  return clampV(vMin + blockCoordinate * 16.0 * vScale);
 }
 [[nodiscard]] double vFromEnd(double blockCoordinate) const noexcept {
  return clampV(vMax - blockCoordinate * 16.0 * vScale);
 }
};
[[nodiscard]] inline const char* atlasPathFor(AtlasDomain domain) noexcept {
 return domain == AtlasDomain::Terrain ? "/terrain.png" : "/gui/items.png";
}
[[nodiscard]] inline ResolvedTexture resolveBlockTextureUv(int textureId) {
 if(net::minecraft::registry::TextureRegistry::isCustomTexture(textureId)) {
  constexpr double modInv = 1.0 / 16.0;
  return {0.0, 1.0, 0.0, 1.0, modInv, modInv, -1, true};
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
         -1,
         false};
}
[[nodiscard]] inline ResolvedTexture resolveBlockTexture(
    int textureId, net::minecraft::client::texture::TextureManager& textureManager, AtlasDomain domain) {
 ResolvedTexture resolved = resolveBlockTextureUv(textureId);
 resolved.glId = resolved.isModTexture
                     ? net::minecraft::registry::TextureRegistry::resolveGlId(textureId, textureManager)
                     : textureManager.getTextureId(atlasPathFor(domain));
 return resolved;
}
} // namespace net::minecraft::client::render
