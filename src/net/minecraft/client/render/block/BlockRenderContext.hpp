#pragma once
#include <cmath>
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/TextureResolve.hpp"
#include "net/minecraft/client/render/block/LeafInteriorFaces.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft {
class BlockView;
class World;
class WorldRegion;
} // namespace net::minecraft
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft::client::render::block {
enum class SideFaceDirection { east,
                               west,
                               north,
                               south };
struct UvCoordinate {
 double u = 0.0;
 double v = 0.0;
};
struct SideFaceUv {
 UvCoordinate vertices[4];
};
inline SideFaceUv sideFaceUvFor(const net::minecraft::client::render::ResolvedTexture& texture,
                                const net::minecraft::Box& bounds,
                                bool flipTextureHorizontally,
                                SideFaceDirection face,
                                int rotation) {
 const bool xAxis = face == SideFaceDirection::east || face == SideFaceDirection::west;
 const double minHorizontal = xAxis ? bounds.minX : bounds.minZ;
 const double maxHorizontal = xAxis ? bounds.maxX : bounds.maxZ;
 const double minU = texture.uFromStart(minHorizontal);
 const double maxU = texture.uFromStart(maxHorizontal);
 double uMin = minU;
 double uMax = maxU;
 double vMin = texture.vFromEnd(bounds.maxY);
 double vMax = texture.vFromEnd(bounds.minY);
 if(flipTextureHorizontally) {
  const double flippedMin = uMin;
  uMin = uMax;
  uMax = flippedMin;
 }
 if(minHorizontal < 0.0 || maxHorizontal > 1.0) {
  uMin = texture.safeUMin();
  uMax = texture.safeUMax();
 }
 if(bounds.minY < 0.0 || bounds.maxY > 1.0) {
  vMin = texture.safeVMin();
  vMax = texture.safeVMax();
 }
 const bool patternA = (face == SideFaceDirection::east || face == SideFaceDirection::south)
                           ? rotation == 2
                           : rotation == 1;
 const bool patternB = (face == SideFaceDirection::east || face == SideFaceDirection::south)
                           ? rotation == 1
                           : rotation == 2;
 UvCoordinate minCorner{uMin, vMin};
 UvCoordinate maxCorner{uMax, vMax};
 UvCoordinate cornerA{uMax, vMin};
 UvCoordinate cornerB{uMin, vMax};
 if(patternA) {
  const double rotatedUMin = texture.uFromStart(bounds.minY);
  const double rotatedUMax = texture.uFromStart(bounds.maxY);
  const double rotatedVMin = texture.vFromEnd(minHorizontal);
  const double rotatedVMax = texture.vFromEnd(maxHorizontal);
  minCorner = {rotatedUMin, rotatedVMax};
  maxCorner = {rotatedUMax, rotatedVMin};
  cornerA = {rotatedUMin, rotatedVMin};
  cornerB = {rotatedUMax, rotatedVMax};
 } else if(patternB) {
  const double rotatedUMin = texture.uFromEnd(bounds.maxY);
  const double rotatedUMax = texture.uFromEnd(bounds.minY);
  const bool reverseHorizontal = face == SideFaceDirection::east || face == SideFaceDirection::south;
  const double rotatedVMin = texture.vFromStart(reverseHorizontal ? maxHorizontal : minHorizontal);
  const double rotatedVMax = texture.vFromStart(reverseHorizontal ? minHorizontal : maxHorizontal);
  minCorner = {rotatedUMax, rotatedVMin};
  maxCorner = {rotatedUMin, rotatedVMax};
  cornerA = {rotatedUMax, rotatedVMax};
  cornerB = {rotatedUMin, rotatedVMin};
 } else if(rotation == 3) {
  const double rotatedUMin = texture.uFromEnd(minHorizontal);
  const double rotatedUMax = texture.uFromEnd(maxHorizontal);
  const double rotatedVMin = texture.vFromStart(bounds.maxY);
  const double rotatedVMax = texture.vFromStart(bounds.minY);
  minCorner = {rotatedUMin, rotatedVMin};
  maxCorner = {rotatedUMax, rotatedVMax};
  cornerA = {rotatedUMax, rotatedVMin};
  cornerB = {rotatedUMin, rotatedVMax};
 }
 SideFaceUv uv;
 if(face == SideFaceDirection::east || face == SideFaceDirection::north) {
  uv.vertices[0] = cornerA;
  uv.vertices[1] = minCorner;
  uv.vertices[2] = cornerB;
  uv.vertices[3] = maxCorner;
 } else if(face == SideFaceDirection::west) {
  uv.vertices[0] = minCorner;
  uv.vertices[1] = cornerB;
  uv.vertices[2] = maxCorner;
  uv.vertices[3] = cornerA;
 } else {
  uv.vertices[0] = cornerB;
  uv.vertices[1] = maxCorner;
  uv.vertices[2] = cornerA;
  uv.vertices[3] = minCorner;
 }
 return uv;
}
inline net::minecraft::block::TerrainAtlasUv tileUvFor(int textureId, const net::minecraft::Box& bounds) {
 if(!net::minecraft::registry::TextureRegistry::isCustomTexture(textureId)) {
  return net::minecraft::block::Block::terrainTileUv(textureId);
 }
 const net::minecraft::client::render::ResolvedTexture texture =
     net::minecraft::client::render::resolveBlockTextureUv(textureId);
 double uMin = texture.uFromStart(bounds.minX);
 double uMax = texture.uFromStart(bounds.maxX);
 double vMin = texture.vFromStart(bounds.minZ);
 double vMax = texture.vFromStart(bounds.maxZ);
 if(bounds.minX < 0.0 || bounds.maxX > 1.0) {
  uMin = texture.uMin;
  uMax = texture.uMax;
 }
 if(bounds.minZ < 0.0 || bounds.maxZ > 1.0) {
  vMin = texture.vMin;
  vMax = texture.vMax;
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
 float alpha[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};
struct BlockFaceRenderState {
 bool useAo = false;
 BlockFaceVertexColors colors;
 // Fractional so a four-neighbour corner average keeps its quarter-levels.
 float blockLight[4] = {15.0f, 15.0f, 15.0f, 15.0f};
 float skyLight[4] = {15.0f, 15.0f, 15.0f, 15.0f};
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
 // Tessellator the renderers emit geometry into. Every producer assigns a
 // concrete instance at construction — chunk-mesh workers pass a private
 // capture-only tessellator, main-thread draws pass thread_local INSTANCE — so
 // no code path can silently reach a global from a worker.
 Tessellator* tess = nullptr;
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
 option::RenderSettings opts{};
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
 // Emit only the leaf-blob interior faces (terrain_layer::CutoutInterior) rather
 // than the ordinary exterior ones. See LeafInteriorFaces.hpp.
 bool interiorFacePass = false;
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
 double blockX = 0.0;
 double blockY = 0.0;
 double blockZ = 0.0;
 int blockEmission = 0;
 int blockLight = 15;
 int skyLight = 15;
 int blockId = 0;
 bool blockFluid = false;
 int blockMetadata = 0;
 // Light the face currently being emitted samples. A solid block stores no
 // light of its own, so the lightmap coordinate for each face has to come from
 // the neighbour that face looks at — exactly where vanilla samples the
 // brightness it used to bake into the vertex colour. Defaults to the block's
 // own light for renderers that draw non-cube geometry.
 const net::minecraft::WorldRegion* lightRegion = nullptr;
 const net::minecraft::World* lightWorld = nullptr;
 const chunk::RegionSnapshot* lightSnapshot = nullptr;
 int faceBlockLight = 15;
 int faceSkyLight = 15;
 // Absolute luminance of the block itself is no longer used to invent relative
 // AO — packs own lighting; C++ writes vanilla corner-averaged light into the
 // per-vertex lightmap coords and keeps the vertex colour tint-only.
 // Set per block by BlockRenderManager::render; cleared by whichever of
 // tessellator() / activeTess() comes first.
 bool surroundingLightPending = false;
 void resolveLightSource();
 // Typed and inline for the mesher's snapshot: the build has no LTO, so the
 // out-of-line virtual dispatch these replace was a real call per sample, and the
 // AO sampler makes dozens per emitted block. Other views take the slow path.
 [[nodiscard]] int blockIdAt(int x, int y, int z) const {
  if(lightSnapshot != nullptr) {
   return lightSnapshot->getBlockId(x, y, z);
  }
  return blockView != nullptr ? blockView->getBlockId(x, y, z) : 0;
 }
 // `blockIdHere` is the block standing at (x,y,z); it decides whether the sample
 // spreads to its neighbours. A caller that already read it passes it rather than
 // making this read it twice.
 void sampleFaceLight(int x, int y, int z, int blockIdHere = -1) {
  if(lightSnapshot == nullptr) {
   sampleFaceLightGeneric(x, y, z);
   return;
  }
  sampleFaceLightWith(
      x, y, z, blockIdHere < 0 ? lightSnapshot->getBlockId(x, y, z) : blockIdHere,
      [this](int sampleX, int sampleY, int sampleZ, int& blockLight, int& skyLight) {
       blockLight = lightSnapshot->getBlockLight(sampleX, sampleY, sampleZ);
       skyLight = lightSnapshot->getSkyLight(sampleX, sampleY, sampleZ);
      });
 }
 // One body, two light sources: the mesher's snapshot binds it inline above, the
 // region/world/no-view cases bind it out of line in sampleFaceLightGeneric. Only
 // the two lines that read the light differ, and they were worth writing twice
 // exactly once -- as a template argument, not as a second copy of the walk.
 template <typename Sample>
 void sampleFaceLightWith(int x, int y, int z, int blockIdHere, Sample&& read) {
  const auto sample = [&read](int sampleX, int sampleY, int sampleZ, int& blockLight, int& skyLight) {
   sampleY = sampleY < 0 ? 0 : (sampleY > net::minecraft::Chunk::height - 1 ? net::minecraft::Chunk::height - 1
                                                                           : sampleY);
   read(sampleX, sampleY, sampleZ, blockLight, skyLight);
  };
  sample(x, y, z, faceBlockLight, faceSkyLight);
  if(net::minecraft::block::Block::usesNeighborLightSampling(blockIdHere)) {
   static constexpr int kOffsets[5][3] = {{0, 1, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};
   for(const auto& offset : kOffsets) {
    int blockLight = 0;
    int skyLight = 0;
    sample(x + offset[0], y + offset[1], z + offset[2], blockLight, skyLight);
    faceBlockLight = faceBlockLight < blockLight ? blockLight : faceBlockLight;
    faceSkyLight = faceSkyLight < skyLight ? skyLight : faceSkyLight;
   }
  }
  faceBlockLight = faceBlockLight < blockEmission ? blockEmission : faceBlockLight;
 }
 void sampleFaceLightGeneric(int x, int y, int z);
 // Default for renderers that emit geometry without telling us which face they
 // are on — baked models and mod blocks. A solid block stores no light of its
 // own, so the surfaces take the brightest light reaching them from around it.
 void sampleSurroundingLight(int x, int y, int z);
 // Renderers that emit geometry without a per-face texture still need a
 // tessellator. The context always carries one (see the tess member comment).
 // Renderers emitting straight into the tessellator need the block's
 // surrounding-light default; ones going through activeTess() overwrite it with the
 // face's own light before their first vertex, so for them its fourteen light reads
 // were pure waste. Sampled on the first call that can observe it, not before.
 [[nodiscard]] Tessellator& tessellator() {
  if(surroundingLightPending) {
   surroundingLightPending = false;
   // blockX/Y/Z are the integer coordinates render() was called with.
   const int x = static_cast<int>(blockX);
   const int y = static_cast<int>(blockY);
   const int z = static_cast<int>(blockZ);
   sampleSurroundingLight(x, y, z);
   blockLight = faceBlockLight;
   skyLight = faceSkyLight;
   tess->blockData(x, y, z, blockEmission, blockLight, skyLight, blockId, blockFluid, blockMetadata);
  }
  return *tess;
 }
 [[nodiscard]] Tessellator& activeTess(int texture) {
  // A face light supersedes the surrounding default, exactly as it did when the
  // default was pushed eagerly and this call overwrote it.
  surroundingLightPending = false;
  if(modMeshes != nullptr) {
   Tessellator& active = modMeshes->tessFor(texture, *tess);
   active.blockData(blockX, blockY, blockZ, blockEmission, faceBlockLight, faceSkyLight, blockId, blockFluid,
                    blockMetadata);
   return active;
  }
  tess->blockData(blockX, blockY, blockZ, blockEmission, faceBlockLight, faceSkyLight, blockId, blockFluid,
                  blockMetadata);
  return *tess;
 }
 // Mod textures each need their own bind; vanilla ids share the already-bound
 // terrain atlas, so leave the binding alone for those.
 void bindTextureFor(int texture) const {
  if(textureManager == nullptr) {
   return;
  }
  const net::minecraft::client::render::ResolvedTexture resolved =
      net::minecraft::client::render::resolveBlockTexture(
          texture, *textureManager, net::minecraft::client::render::AtlasDomain::Terrain);
  if(resolved.isModTexture) {
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
  // The two passes partition a leaf block's faces: the exterior pass keeps the
  // vanilla rule (leaf-against-leaf culled), the interior pass emits nothing
  // but those culled boundaries, one quad each.
  if(interiorFacePass) {
   return leafInteriorFaceVisible(blockView, block, x, y, z, side, opts.leafInteriorFaces);
  }
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
