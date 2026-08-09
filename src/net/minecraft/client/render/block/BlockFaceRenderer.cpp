#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
namespace net::minecraft::client::render::block {
namespace {
int beginFace(BlockRenderContext& ctx, int side, int texture, Tessellator*& tessellator) {
 texture = ctx.resolveTexture(side, texture);
 ctx.bindTextureFor(texture);
 tessellator = &ctx.activeTess(texture);
 return texture;
}
void emitVertex(
    Tessellator& tessellator, float nx, float ny, float nz, double x, double y, double z, double u, double v) {
 emitBlockVertex(tessellator, nx, ny, nz, x, y, z, u, v);
}
void emitAoCorner(BlockRenderContext& ctx,
                  Tessellator& tessellator,
                  int corner,
                  float nx,
                  float ny,
                  float nz,
                  double x,
                  double y,
                  double z,
                  double u,
                  double v) {
 tessellator.light(ctx.faceState.blockLight[corner], ctx.faceState.skyLight[corner]);
 tessellator.color(ctx.faceState.colors.red[corner], ctx.faceState.colors.green[corner],
                   ctx.faceState.colors.blue[corner], ctx.faceState.colors.alpha[corner]);
 emitVertex(tessellator, nx, ny, nz, x, y, z, u, v);
}
} // namespace
void BlockFaceRenderer::renderBottomFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
 Tessellator* tessellator = nullptr;
 texture = beginFace(ctx_, 0, texture, tessellator);
 const net::minecraft::client::render::ResolvedTexture textureUv =
     net::minecraft::client::render::resolveBlockTextureUv(texture);
 double uMin = textureUv.uFromStart(ctx_.renderBounds.minX);
 double uMax = textureUv.uFromStart(ctx_.renderBounds.maxX);
 double vMin = textureUv.vFromStart(ctx_.renderBounds.minZ);
 double vMax = textureUv.vFromStart(ctx_.renderBounds.maxZ);
 if(ctx_.renderBounds.minX < 0.0 || ctx_.renderBounds.maxX > 1.0) {
  uMin = textureUv.safeUMin();
  uMax = textureUv.safeUMax();
 }
 if(ctx_.renderBounds.minZ < 0.0 || ctx_.renderBounds.maxZ > 1.0) {
  vMin = textureUv.safeVMin();
  vMax = textureUv.safeVMax();
 }
 double uCornerA = uMax;
 double uCornerB = uMin;
 double vCornerA = vMin;
 double vCornerB = vMax;
 if(ctx_.bottomFaceRotation == 2) {
  uMin = textureUv.uFromStart(ctx_.renderBounds.minZ);
  vMin = textureUv.vFromEnd(ctx_.renderBounds.maxX);
  uMax = textureUv.uFromStart(ctx_.renderBounds.maxZ);
  vMax = textureUv.vFromEnd(ctx_.renderBounds.minX);
  uCornerA = uMax;
  uCornerB = uMin;
  vCornerA = vMin;
  vCornerB = vMax;
  uCornerA = uMin;
  uCornerB = uMax;
  vMin = vMax;
  vMax = vCornerA;
 } else if(ctx_.bottomFaceRotation == 1) {
  uMin = textureUv.uFromEnd(ctx_.renderBounds.maxZ);
  vMin = textureUv.vFromStart(ctx_.renderBounds.minX);
  uMax = textureUv.uFromEnd(ctx_.renderBounds.minZ);
  vMax = textureUv.vFromStart(ctx_.renderBounds.maxX);
  uCornerA = uMax;
  uCornerB = uMin;
  vCornerA = vMin;
  vCornerB = vMax;
  uMin = uCornerA;
  uMax = uCornerB;
  vCornerA = vMax;
  vCornerB = vMin;
 } else if(ctx_.bottomFaceRotation == 3) {
  uMin = textureUv.uFromEnd(ctx_.renderBounds.minX);
  uMax = textureUv.uFromEnd(ctx_.renderBounds.maxX);
  vMin = textureUv.vFromEnd(ctx_.renderBounds.minZ);
  vMax = textureUv.vFromEnd(ctx_.renderBounds.maxZ);
  uCornerA = uMax;
  uCornerB = uMin;
  vCornerA = vMin;
  vCornerB = vMax;
 }
 const double xMin = x + ctx_.renderBounds.minX;
 const double xMax = x + ctx_.renderBounds.maxX;
 const double yCoord = y + ctx_.renderBounds.minY;
 const double zMin = z + ctx_.renderBounds.minZ;
 const double zMax = z + ctx_.renderBounds.maxZ;
 if(ctx_.faceState.useAo) {
  emitAoCorner(ctx_, *tessellator, 0, 0.0f, -1.0f, 0.0f, xMin, yCoord, zMax, uCornerB, vCornerB);
  emitAoCorner(ctx_, *tessellator, 1, 0.0f, -1.0f, 0.0f, xMin, yCoord, zMin, uMin, vMin);
  emitAoCorner(ctx_, *tessellator, 2, 0.0f, -1.0f, 0.0f, xMax, yCoord, zMin, uCornerA, vCornerA);
  emitAoCorner(ctx_, *tessellator, 3, 0.0f, -1.0f, 0.0f, xMax, yCoord, zMax, uMax, vMax);
 } else {
  emitVertex(*tessellator, 0.0f, -1.0f, 0.0f, xMin, yCoord, zMax, uCornerB, vCornerB);
  emitVertex(*tessellator, 0.0f, -1.0f, 0.0f, xMin, yCoord, zMin, uMin, vMin);
  emitVertex(*tessellator, 0.0f, -1.0f, 0.0f, xMax, yCoord, zMin, uCornerA, vCornerA);
  emitVertex(*tessellator, 0.0f, -1.0f, 0.0f, xMax, yCoord, zMax, uMax, vMax);
 }
}
void BlockFaceRenderer::renderTopFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
 Tessellator* tessellator = nullptr;
 texture = beginFace(ctx_, 1, texture, tessellator);
 const net::minecraft::client::render::ResolvedTexture textureUv =
     net::minecraft::client::render::resolveBlockTextureUv(texture);
 double uMin = textureUv.uFromStart(ctx_.renderBounds.minX);
 double uMax = textureUv.uFromStart(ctx_.renderBounds.maxX);
 double vMin = textureUv.vFromStart(ctx_.renderBounds.minZ);
 double vMax = textureUv.vFromStart(ctx_.renderBounds.maxZ);
 if(ctx_.renderBounds.minX < 0.0 || ctx_.renderBounds.maxX > 1.0) {
  uMin = textureUv.safeUMin();
  uMax = textureUv.safeUMax();
 }
 if(ctx_.renderBounds.minZ < 0.0 || ctx_.renderBounds.maxZ > 1.0) {
  vMin = textureUv.safeVMin();
  vMax = textureUv.safeVMax();
 }
 double uCornerA = uMax;
 double uCornerB = uMin;
 double vCornerA = vMin;
 double vCornerB = vMax;
 if(ctx_.topFaceRotation == 1) {
  uMin = textureUv.uFromStart(ctx_.renderBounds.minZ);
  vMin = textureUv.vFromEnd(ctx_.renderBounds.maxX);
  uMax = textureUv.uFromStart(ctx_.renderBounds.maxZ);
  vMax = textureUv.vFromEnd(ctx_.renderBounds.minX);
  uCornerA = uMax;
  uCornerB = uMin;
  vCornerA = vMin;
  vCornerB = vMax;
  uCornerA = uMin;
  uCornerB = uMax;
  vMin = vMax;
  vMax = vCornerA;
 } else if(ctx_.topFaceRotation == 2) {
  uMin = textureUv.uFromEnd(ctx_.renderBounds.maxZ);
  vMin = textureUv.vFromStart(ctx_.renderBounds.minX);
  uMax = textureUv.uFromEnd(ctx_.renderBounds.minZ);
  vMax = textureUv.vFromStart(ctx_.renderBounds.maxX);
  uCornerA = uMax;
  uCornerB = uMin;
  vCornerA = vMin;
  vCornerB = vMax;
  uMin = uCornerA;
  uMax = uCornerB;
  vCornerA = vMax;
  vCornerB = vMin;
 } else if(ctx_.topFaceRotation == 3) {
  uMin = textureUv.uFromEnd(ctx_.renderBounds.minX);
  uMax = textureUv.uFromEnd(ctx_.renderBounds.maxX);
  vMin = textureUv.vFromEnd(ctx_.renderBounds.minZ);
  vMax = textureUv.vFromEnd(ctx_.renderBounds.maxZ);
  uCornerA = uMax;
  uCornerB = uMin;
  vCornerA = vMin;
  vCornerB = vMax;
 }
 const double xMin = x + ctx_.renderBounds.minX;
 const double xMax = x + ctx_.renderBounds.maxX;
 const double yCoord = y + ctx_.renderBounds.maxY;
 const double zMin = z + ctx_.renderBounds.minZ;
 const double zMax = z + ctx_.renderBounds.maxZ;
 if(ctx_.faceState.useAo) {
  emitAoCorner(ctx_, *tessellator, 0, 0.0f, 1.0f, 0.0f, xMax, yCoord, zMax, uMax, vMax);
  emitAoCorner(ctx_, *tessellator, 1, 0.0f, 1.0f, 0.0f, xMax, yCoord, zMin, uCornerA, vCornerA);
  emitAoCorner(ctx_, *tessellator, 2, 0.0f, 1.0f, 0.0f, xMin, yCoord, zMin, uMin, vMin);
  emitAoCorner(ctx_, *tessellator, 3, 0.0f, 1.0f, 0.0f, xMin, yCoord, zMax, uCornerB, vCornerB);
 } else {
  emitVertex(*tessellator, 0.0f, 1.0f, 0.0f, xMax, yCoord, zMax, uMax, vMax);
  emitVertex(*tessellator, 0.0f, 1.0f, 0.0f, xMax, yCoord, zMin, uCornerA, vCornerA);
  emitVertex(*tessellator, 0.0f, 1.0f, 0.0f, xMin, yCoord, zMin, uMin, vMin);
  emitVertex(*tessellator, 0.0f, 1.0f, 0.0f, xMin, yCoord, zMax, uCornerB, vCornerB);
 }
}
void BlockFaceRenderer::renderEastFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
 Tessellator* tessellator = nullptr;
 texture = beginFace(ctx_, 2, texture, tessellator);
 const net::minecraft::client::render::ResolvedTexture textureUv =
     net::minecraft::client::render::resolveBlockTextureUv(texture);
 const SideFaceUv uv = sideFaceUvFor(textureUv, ctx_.renderBounds, ctx_.flipTextureHorizontally,
                                     SideFaceDirection::east, ctx_.eastFaceRotation);
 const double xMin = x + ctx_.renderBounds.minX;
 const double xMax = x + ctx_.renderBounds.maxX;
 const double yMin = y + ctx_.renderBounds.minY;
 const double yMax = y + ctx_.renderBounds.maxY;
 const double zCoord = z + ctx_.renderBounds.minZ;
 if(ctx_.faceState.useAo) {
  emitAoCorner(ctx_, *tessellator, 0, 0.0f, 0.0f, -1.0f, xMin, yMax, zCoord, uv.vertices[0].u, uv.vertices[0].v);
  emitAoCorner(ctx_, *tessellator, 1, 0.0f, 0.0f, -1.0f, xMax, yMax, zCoord, uv.vertices[1].u, uv.vertices[1].v);
  emitAoCorner(ctx_, *tessellator, 2, 0.0f, 0.0f, -1.0f, xMax, yMin, zCoord, uv.vertices[2].u, uv.vertices[2].v);
  emitAoCorner(ctx_, *tessellator, 3, 0.0f, 0.0f, -1.0f, xMin, yMin, zCoord, uv.vertices[3].u, uv.vertices[3].v);
 } else {
  emitVertex(*tessellator, 0.0f, 0.0f, -1.0f, xMin, yMax, zCoord, uv.vertices[0].u, uv.vertices[0].v);
  emitVertex(*tessellator, 0.0f, 0.0f, -1.0f, xMax, yMax, zCoord, uv.vertices[1].u, uv.vertices[1].v);
  emitVertex(*tessellator, 0.0f, 0.0f, -1.0f, xMax, yMin, zCoord, uv.vertices[2].u, uv.vertices[2].v);
  emitVertex(*tessellator, 0.0f, 0.0f, -1.0f, xMin, yMin, zCoord, uv.vertices[3].u, uv.vertices[3].v);
 }
}
void BlockFaceRenderer::renderWestFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
 Tessellator* tessellator = nullptr;
 texture = beginFace(ctx_, 3, texture, tessellator);
 const net::minecraft::client::render::ResolvedTexture textureUv =
     net::minecraft::client::render::resolveBlockTextureUv(texture);
 const SideFaceUv uv = sideFaceUvFor(textureUv, ctx_.renderBounds, ctx_.flipTextureHorizontally,
                                     SideFaceDirection::west, ctx_.westFaceRotation);
 const double xMin = x + ctx_.renderBounds.minX;
 const double xMax = x + ctx_.renderBounds.maxX;
 const double yMin = y + ctx_.renderBounds.minY;
 const double yMax = y + ctx_.renderBounds.maxY;
 const double zCoord = z + ctx_.renderBounds.maxZ;
 if(ctx_.faceState.useAo) {
  emitAoCorner(ctx_, *tessellator, 0, 0.0f, 0.0f, 1.0f, xMin, yMax, zCoord, uv.vertices[0].u, uv.vertices[0].v);
  emitAoCorner(ctx_, *tessellator, 1, 0.0f, 0.0f, 1.0f, xMin, yMin, zCoord, uv.vertices[1].u, uv.vertices[1].v);
  emitAoCorner(ctx_, *tessellator, 2, 0.0f, 0.0f, 1.0f, xMax, yMin, zCoord, uv.vertices[2].u, uv.vertices[2].v);
  emitAoCorner(ctx_, *tessellator, 3, 0.0f, 0.0f, 1.0f, xMax, yMax, zCoord, uv.vertices[3].u, uv.vertices[3].v);
 } else {
  emitVertex(*tessellator, 0.0f, 0.0f, 1.0f, xMin, yMax, zCoord, uv.vertices[0].u, uv.vertices[0].v);
  emitVertex(*tessellator, 0.0f, 0.0f, 1.0f, xMin, yMin, zCoord, uv.vertices[1].u, uv.vertices[1].v);
  emitVertex(*tessellator, 0.0f, 0.0f, 1.0f, xMax, yMin, zCoord, uv.vertices[2].u, uv.vertices[2].v);
  emitVertex(*tessellator, 0.0f, 0.0f, 1.0f, xMax, yMax, zCoord, uv.vertices[3].u, uv.vertices[3].v);
 }
}
void BlockFaceRenderer::renderNorthFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
 Tessellator* tessellator = nullptr;
 texture = beginFace(ctx_, 4, texture, tessellator);
 const net::minecraft::client::render::ResolvedTexture textureUv =
     net::minecraft::client::render::resolveBlockTextureUv(texture);
 const SideFaceUv uv = sideFaceUvFor(textureUv, ctx_.renderBounds, ctx_.flipTextureHorizontally,
                                     SideFaceDirection::north, ctx_.northFaceRotation);
 const double xCoord = x + ctx_.renderBounds.minX;
 const double yMin = y + ctx_.renderBounds.minY;
 const double yMax = y + ctx_.renderBounds.maxY;
 const double zMin = z + ctx_.renderBounds.minZ;
 const double zMax = z + ctx_.renderBounds.maxZ;
 if(ctx_.faceState.useAo) {
  emitAoCorner(ctx_, *tessellator, 0, -1.0f, 0.0f, 0.0f, xCoord, yMax, zMax, uv.vertices[0].u, uv.vertices[0].v);
  emitAoCorner(ctx_, *tessellator, 1, -1.0f, 0.0f, 0.0f, xCoord, yMax, zMin, uv.vertices[1].u, uv.vertices[1].v);
  emitAoCorner(ctx_, *tessellator, 2, -1.0f, 0.0f, 0.0f, xCoord, yMin, zMin, uv.vertices[2].u, uv.vertices[2].v);
  emitAoCorner(ctx_, *tessellator, 3, -1.0f, 0.0f, 0.0f, xCoord, yMin, zMax, uv.vertices[3].u, uv.vertices[3].v);
 } else {
  emitVertex(*tessellator, -1.0f, 0.0f, 0.0f, xCoord, yMax, zMax, uv.vertices[0].u, uv.vertices[0].v);
  emitVertex(*tessellator, -1.0f, 0.0f, 0.0f, xCoord, yMax, zMin, uv.vertices[1].u, uv.vertices[1].v);
  emitVertex(*tessellator, -1.0f, 0.0f, 0.0f, xCoord, yMin, zMin, uv.vertices[2].u, uv.vertices[2].v);
  emitVertex(*tessellator, -1.0f, 0.0f, 0.0f, xCoord, yMin, zMax, uv.vertices[3].u, uv.vertices[3].v);
 }
}
void BlockFaceRenderer::renderSouthFace(
    net::minecraft::block::Block& /*block*/, double x, double y, double z, int texture) {
 Tessellator* tessellator = nullptr;
 texture = beginFace(ctx_, 5, texture, tessellator);
 const net::minecraft::client::render::ResolvedTexture textureUv =
     net::minecraft::client::render::resolveBlockTextureUv(texture);
 const SideFaceUv uv = sideFaceUvFor(textureUv, ctx_.renderBounds, ctx_.flipTextureHorizontally,
                                     SideFaceDirection::south, ctx_.southFaceRotation);
 const double xCoord = x + ctx_.renderBounds.maxX;
 const double yMin = y + ctx_.renderBounds.minY;
 const double yMax = y + ctx_.renderBounds.maxY;
 const double zMin = z + ctx_.renderBounds.minZ;
 const double zMax = z + ctx_.renderBounds.maxZ;
 if(ctx_.faceState.useAo) {
  emitAoCorner(ctx_, *tessellator, 0, 1.0f, 0.0f, 0.0f, xCoord, yMin, zMax, uv.vertices[0].u, uv.vertices[0].v);
  emitAoCorner(ctx_, *tessellator, 1, 1.0f, 0.0f, 0.0f, xCoord, yMin, zMin, uv.vertices[1].u, uv.vertices[1].v);
  emitAoCorner(ctx_, *tessellator, 2, 1.0f, 0.0f, 0.0f, xCoord, yMax, zMin, uv.vertices[2].u, uv.vertices[2].v);
  emitAoCorner(ctx_, *tessellator, 3, 1.0f, 0.0f, 0.0f, xCoord, yMax, zMax, uv.vertices[3].u, uv.vertices[3].v);
 } else {
  emitVertex(*tessellator, 1.0f, 0.0f, 0.0f, xCoord, yMin, zMax, uv.vertices[0].u, uv.vertices[0].v);
  emitVertex(*tessellator, 1.0f, 0.0f, 0.0f, xCoord, yMin, zMin, uv.vertices[1].u, uv.vertices[1].v);
  emitVertex(*tessellator, 1.0f, 0.0f, 0.0f, xCoord, yMax, zMin, uv.vertices[2].u, uv.vertices[2].v);
  emitVertex(*tessellator, 1.0f, 0.0f, 0.0f, xCoord, yMax, zMax, uv.vertices[3].u, uv.vertices[3].v);
 }
}
} // namespace net::minecraft::client::render::block
