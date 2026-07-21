#pragma once
#include <cmath>
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/client/render/TextureResolve.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft {
class BlockView;
}
#include "net/minecraft/block/Block.hpp"
namespace net::minecraft::client::render::block {

struct TileScale {
 int u = 0;
 int v = 0;
 double inv = 1.0 / 256.0;
};

inline TileScale tileScaleFor(int textureId) {
 const net::minecraft::client::render::ResolvedTexture uv =
     net::minecraft::client::render::resolveBlockTextureUv(textureId);
 if(net::minecraft::registry::TextureRegistry::isCustomTexture(textureId)) {
  return {0, 0, uv.uScale};
 }
 return {
  net::minecraft::block::Block::textureAtlasU(textureId),
  net::minecraft::block::Block::textureAtlasV(textureId),
  uv.uScale
 };
}

inline net::minecraft::block::TerrainAtlasUv tileUvFor(int textureId, const net::minecraft::Box& bounds) {
 if(!net::minecraft::registry::TextureRegistry::isCustomTexture(textureId)) {
  return net::minecraft::block::Block::terrainTileUv(textureId);
 }
 const TileScale tile = tileScaleFor(textureId);
 double uMin = (static_cast<double>(tile.u) + bounds.minX * 16.0) * tile.inv;
 double uMax = (static_cast<double>(tile.u) + bounds.maxX * 16.0) * tile.inv;
 double vMin = (static_cast<double>(tile.v) + bounds.minZ * 16.0) * tile.inv;
 double vMax = (static_cast<double>(tile.v) + bounds.maxZ * 16.0) * tile.inv;
 if(bounds.minX < 0.0 || bounds.maxX > 1.0) {
  uMin = static_cast<double>(tile.u) * tile.inv;
  uMax = static_cast<double>(tile.u + 16) * tile.inv;
 }
 if(bounds.minZ < 0.0 || bounds.maxZ > 1.0) {
  vMin = static_cast<double>(tile.v) * tile.inv;
  vMax = static_cast<double>(tile.v + 16) * tile.inv;
 }
 return {uMin, uMax, vMin, vMax};
}

inline net::minecraft::block::TerrainAtlasUv tileUvFor(int textureId) {
 return tileUvFor(textureId, net::minecraft::Box{0.0, 0.0, 0.0, 1.0, 1.0, 1.0});
}

struct BlockFaceVertexColors {
 float red[4] = {1.0f, 1.0f, 1.0f, 1.0f};
 float green[4] = {1.0f, 1.0f, 1.0f, 1.0f};
 float blue[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};
struct BlockFaceRenderState {
 bool useAo = false;
 BlockFaceVertexColors colors;
};
// Mutable rendering state shared by the cooperating block renderers (faces,
// cube, fluid, plants, mechanisms, inventory icon). In the original beta 1.7.3
// RenderBlocks these were all fields of one god-class; pulling them into a
// context lets the renderers be split into focused classes while preserving the
// exact cross-method communication they relied on (texture overrides, face
// rotations, culling toggles, AO colours, ...).
struct BlockRenderContext {
 // World/chunk the blocks are read from while rendering in-place.
 const net::minecraft::BlockView* blockView = nullptr;
 // Tessellator the renderers emit geometry into. Defaults to the shared
 // main-thread instance; chunk-mesh worker jobs point this at a private
 // capture-only tessellator so meshing never touches GL or shared state.
 Tessellator* tess = &Tessellator::INSTANCE;
 // Bounds of the block currently being rendered, in local 0..1 block space.
 // Owned by the context (not the Block singleton) so concurrent mesh jobs
 // and the main-thread tick can't race on Block::minX..maxZ.
 Box renderBounds{0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
 void setRenderBounds(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) noexcept {
  renderBounds = Box{minX, minY, minZ, maxX, maxY, maxZ};
 }
 // Render options snapshotted when the owning BlockRenderManager is created
 // (or when a mesh job is enqueued), so renderers never read the live
 // GameOptions from a worker thread.
 option::ResolvedRenderOptions opts{};
 // When >= 0, forces every face to this atlas tile (used by levers, fire,
 // redstone, ... that re-skin a block).
 int textureOverride = -1;
 // When >= 0, replaces only faceTextureSide (0-5) — e.g. piston head top
 // during block-entity animation without retexturing the whole block.
 int faceTextureOverride = -1;
 int faceTextureSide = -1;
 // Mirrors a face's U axis (beds, some directional faces).
 bool flipTextureHorizontally = false;
 // Skip the neighbour-visibility test and always emit faces (item models,
 // extended pistons, ...).
 bool skipFaceCulling = false;
 // Per-face 90-degree texture rotations (0..3), set by directional blocks.
 int eastFaceRotation = 0;
 int westFaceRotation = 0;
 int southFaceRotation = 0;
 int northFaceRotation = 0;
 int topFaceRotation = 0;
 int bottomFaceRotation = 0;
 // Smooth-lighting vertex colours produced for the face currently being drawn.
 BlockFaceRenderState faceState;
 // Apply the block's per-metadata colour multiplier when drawing inventory
 // icons (disabled by callers that tint themselves).
 bool inventoryColorEnabled = true;
 client::texture::TextureManager* textureManager = nullptr;
 chunk::ModMeshCollector* modMeshes = nullptr;
 [[nodiscard]] Tessellator& activeTess(int texture) {
  if(tess == nullptr) {
   return Tessellator::INSTANCE;
  }
  if(modMeshes != nullptr) {
   return modMeshes->tessFor(texture, *tess);
  }
  return *tess;
 }
 void bindTextureFor(int texture) const {
  if(textureManager != nullptr && net::minecraft::registry::TextureRegistry::isCustomTexture(texture)) {
   const net::minecraft::client::render::ResolvedTexture resolved =
       net::minecraft::client::render::resolveBlockTexture(
           texture, *textureManager, net::minecraft::client::render::AtlasDomain::Terrain);
   textureManager->bindTexture(resolved.glId);
  }
 }
 [[nodiscard]] int resolveTexture(int side, int texture) const {
  if(faceTextureOverride >= 0 && side == faceTextureSide) {
   return faceTextureOverride;
  }
  if(textureOverride >= 0) {
   return textureOverride;
  }
  return texture;
 }
 [[nodiscard]] bool isSideVisible(const net::minecraft::block::Block& block, int x, int y, int z, int side) const {
  return block.isSideVisibleForBounds(blockView, x, y, z, side, renderBounds);
 }
};
inline void emitBlockVertex(
    Tessellator& tess, float nx, float ny, float nz, double x, double y, double z, double u, double v) {
 tess.normal(nx, ny, nz);
 tess.vertex(x, y, z, u, v);
}
inline void quadNormal(double ax,
                       double ay,
                       double az,
                       double bx,
                       double by,
                       double bz,
                       double cx,
                       double cy,
                       double cz,
                       float& nx,
                       float& ny,
                       float& nz) noexcept {
 const double e1x = bx - ax;
 const double e1y = by - ay;
 const double e1z = bz - az;
 const double e2x = cx - ax;
 const double e2y = cy - ay;
 const double e2z = cz - az;
 const double rawX = e1y * e2z - e1z * e2y;
 const double rawY = e1z * e2x - e1x * e2z;
 const double rawZ = e1x * e2y - e1y * e2x;
 const double len = std::sqrt(rawX * rawX + rawY * rawY + rawZ * rawZ);
 if(len < 1.0e-8) {
  nx = 0.0f;
  ny = 1.0f;
  nz = 0.0f;
  return;
 }
 const float inv = static_cast<float>(1.0 / len);
 nx = static_cast<float>(rawX) * inv;
 ny = static_cast<float>(rawY) * inv;
 nz = static_cast<float>(rawZ) * inv;
}
} // namespace net::minecraft::client::render::block
