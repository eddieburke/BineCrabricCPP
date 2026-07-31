#include <algorithm>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/WorldRegion.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightType.hpp"
namespace net::minecraft::client::render::block {
namespace option = net::minecraft::client::option;
namespace {
bool edgeAllowsVision(const net::minecraft::BlockView* blockView, int x, int y, int z) {
 if(blockView == nullptr) {
  return true;
 }
 const int blockId = blockView->getBlockId(x, y, z);
 if(blockId <= 0 || blockId >= net::minecraft::block::Block::BLOCK_COUNT) {
  return true;
 }
 return net::minecraft::block::Block::BLOCKS_ALLOW_VISION[static_cast<std::size_t>(blockId)];
}
float opacityAo(const net::minecraft::BlockView* blockView, int x, int y, int z) {
 return edgeAllowsVision(blockView, x, y, z) ? 1.0f : 0.2f;
}
void readBlockSky(BlockRenderContext& ctx, int x, int y, int z, int& blockLight, int& skyLight) {
 ctx.sampleFaceLight(x, y, z);
 blockLight = ctx.faceBlockLight;
 skyLight = ctx.faceSkyLight;
}
void averageCornerLight(BlockRenderContext& ctx,
                        int x0,
                        int y0,
                        int z0,
                        int x1,
                        int y1,
                        int z1,
                        int x2,
                        int y2,
                        int z2,
                        int x3,
                        int y3,
                        int z3,
                        int& blockLight,
                        int& skyLight) {
 int b0 = 0;
 int b1 = 0;
 int b2 = 0;
 int b3 = 0;
 int s0 = 0;
 int s1 = 0;
 int s2 = 0;
 int s3 = 0;
 readBlockSky(ctx, x0, y0, z0, b0, s0);
 readBlockSky(ctx, x1, y1, z1, b1, s1);
 readBlockSky(ctx, x2, y2, z2, b2, s2);
 readBlockSky(ctx, x3, y3, z3, b3, s3);
 blockLight = (b0 + b1 + b2 + b3) / 4;
 skyLight = (s0 + s1 + s2 + s3) / 4;
}
void assignAoVertex(const option::RenderSettings& resolved,
                    BlockFaceRenderState& state,
                    int corner,
                    float ao,
                    bool applyTint,
                    float red,
                    float green,
                    float blue) {
 ao = option::scaleAoCorner(ao, resolved);
 const float tintRed = applyTint ? red : 1.0f;
 const float tintGreen = applyTint ? green : 1.0f;
 const float tintBlue = applyTint ? blue : 1.0f;
 if(resolved.separateAo) {
  state.colors.red[corner] = tintRed;
  state.colors.green[corner] = tintGreen;
  state.colors.blue[corner] = tintBlue;
  state.colors.alpha[corner] = ao;
 } else {
  state.colors.red[corner] = tintRed * ao;
  state.colors.green[corner] = tintGreen * ao;
  state.colors.blue[corner] = tintBlue * ao;
  state.colors.alpha[corner] = 1.0f;
 }
}
float oldLightingFaceShade(int face) {
 switch(face) {
 case 0:
  return 0.5f;
 case 1:
  return 1.0f;
 case 2:
 case 3:
  return 0.8f;
 default:
  return 0.6f;
 }
}
void applyOldLightingIfEnabled(const option::RenderSettings& resolved, BlockFaceRenderState& state, int face) {
 if(!resolved.oldLighting) {
  return;
 }
 const float shade = oldLightingFaceShade(face);
 for(int i = 0; i < 4; ++i) {
  state.colors.red[i] *= shade;
  state.colors.green[i] *= shade;
  state.colors.blue[i] *= shade;
 }
}
float flatFaceColor(const option::RenderSettings& resolved, int face, float channel) {
 return resolved.oldLighting ? channel * oldLightingFaceShade(face) : channel;
}
void assignAoCorners(const option::RenderSettings& resolved,
                     BlockFaceRenderState& state,
                     int face,
                     float c0,
                     float c1,
                     float c2,
                     float c3,
                     bool applyTint,
                     float red,
                     float green,
                     float blue,
                     int b0,
                     int s0,
                     int b1,
                     int s1,
                     int b2,
                     int s2,
                     int b3,
                     int s3) {
 assignAoVertex(resolved, state, 0, c0, applyTint, red, green, blue);
 assignAoVertex(resolved, state, 1, c1, applyTint, red, green, blue);
 assignAoVertex(resolved, state, 2, c2, applyTint, red, green, blue);
 assignAoVertex(resolved, state, 3, c3, applyTint, red, green, blue);
 state.blockLight[0] = b0;
 state.skyLight[0] = s0;
 state.blockLight[1] = b1;
 state.skyLight[1] = s1;
 state.blockLight[2] = b2;
 state.skyLight[2] = s2;
 state.blockLight[3] = b3;
 state.skyLight[3] = s3;
 applyOldLightingIfEnabled(resolved, state, face);
}
void multiplyAoVertexColors(BlockFaceRenderState& state, float red, float green, float blue) {
 for(int i = 0; i < 4; ++i) {
  state.colors.red[i] *= red;
  state.colors.green[i] *= green;
  state.colors.blue[i] *= blue;
 }
}
[[nodiscard]] bool grassSideTintActive(const option::RenderSettings& resolved) {
 return resolved.fancyGrass;
}
} // namespace
bool CubeBlockRenderer::renderBlock(net::minecraft::block::Block& block, int x, int y, int z) {
 int n = block.getColorMultiplier(ctx_.blockView, x, y, z);
 float red = (float)(n >> 16 & 0xFF) / 255.0f;
 float green = (float)(n >> 8 & 0xFF) / 255.0f;
 float blue = (float)(n & 0xFF) / 255.0f;
 // Snapshotted at job-enqueue time (or by snapshotGlobals() on the main thread).
 // Never read Minecraft::INSTANCE here: this runs on chunk-mesh worker threads.
 if(ctx_.opts.ambientOcclusionActive) {
  return renderSmooth(block, x, y, z, red, green, blue);
 }
 return renderFlat(block, x, y, z, red, green, blue);
}
// Smooth lighting: C++ writes opacity AO into vaColor and per-vertex lightmap
// averages into vaUV2. Packs own lighting application (face shade, lightmap sample).
bool CubeBlockRenderer::renderSmooth(
    net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue) {
 if(ctx_.blockView == nullptr) {
  return false;
 }
 int textureId = 0;
 ctx_.faceState.useAo = true;
 bool drewAnyFace = false;
 float corner0 = 0.0f;
 float corner1 = 0.0f;
 float corner2 = 0.0f;
 float corner3 = 0.0f;
 int lb0 = 15;
 int ls0 = 15;
 int lb1 = 15;
 int ls1 = 15;
 int lb2 = 15;
 int ls2 = 15;
 int lb3 = 15;
 int ls3 = 15;
 bool tintDown = true;
 bool tintUp = true;
 bool tintEast = true;
 bool tintWest = true;
 bool tintNorth = true;
 bool tintSouth = true;
 const bool topEastEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x + 1, y + 1, z);
 const bool bottomEastEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x + 1, y - 1, z);
 const bool southEastEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x + 1, y, z + 1);
 const bool northEastEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x + 1, y, z - 1);
 const bool topWestEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x - 1, y + 1, z);
 const bool bottomWestEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x - 1, y - 1, z);
 const bool northWestEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x - 1, y, z - 1);
 const bool southWestEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x - 1, y, z + 1);
 const bool topSouthEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x, y + 1, z + 1);
 const bool topNorthEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x, y + 1, z - 1);
 const bool bottomSouthEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x, y - 1, z + 1);
 const bool bottomNorthEdgeTranslucent = edgeAllowsVision(ctx_.blockView, x, y - 1, z - 1);
 if(block.textureId == 3) {
  tintSouth = false;
  tintNorth = false;
  tintWest = false;
  tintEast = false;
  tintDown = false;
 }
 if(ctx_.textureOverride >= 0) {
  tintSouth = false;
  tintNorth = false;
  tintWest = false;
  tintEast = false;
  tintDown = false;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y - 1, z, 0)) {
  const float n = opacityAo(ctx_.blockView, x - 1, y - 1, z);
  const float e = opacityAo(ctx_.blockView, x, y - 1, z - 1);
  const float w = opacityAo(ctx_.blockView, x, y - 1, z + 1);
  const float s = opacityAo(ctx_.blockView, x + 1, y - 1, z);
  const float face = opacityAo(ctx_.blockView, x, y - 1, z);
  const float ne = bottomNorthEdgeTranslucent || bottomWestEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x - 1, y - 1, z - 1)
                       : n;
  const float nw = bottomSouthEdgeTranslucent || bottomWestEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x - 1, y - 1, z + 1)
                       : n;
  const float se = bottomNorthEdgeTranslucent || bottomEastEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x + 1, y - 1, z - 1)
                       : s;
  const float sw = bottomSouthEdgeTranslucent || bottomEastEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x + 1, y - 1, z + 1)
                       : s;
  corner0 = (nw + n + w + face) / 4.0f;
  corner3 = (w + face + sw + s) / 4.0f;
  corner2 = (face + e + s + se) / 4.0f;
  corner1 = (n + ne + face + e) / 4.0f;
  averageCornerLight(ctx_, x - 1, y - 1, z + 1, x - 1, y - 1, z, x, y - 1, z + 1, x, y - 1, z, lb0, ls0);
  averageCornerLight(ctx_, x - 1, y - 1, z, x - 1, y - 1, z - 1, x, y - 1, z, x, y - 1, z - 1, lb1, ls1);
  averageCornerLight(ctx_, x, y - 1, z, x, y - 1, z - 1, x + 1, y - 1, z, x + 1, y - 1, z - 1, lb2, ls2);
  averageCornerLight(ctx_, x, y - 1, z + 1, x, y - 1, z, x + 1, y - 1, z + 1, x + 1, y - 1, z, lb3, ls3);
  assignAoCorners(ctx_.opts, ctx_.faceState, 0, corner0, corner1, corner2, corner3, tintDown, red, green, blue, lb0,
                  ls0, lb1, ls1, lb2, ls2, lb3, ls3);
  faces_.renderBottomFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 0));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y + 1, z, 1)) {
  const float n = opacityAo(ctx_.blockView, x - 1, y + 1, z);
  const float s = opacityAo(ctx_.blockView, x + 1, y + 1, z);
  const float e = opacityAo(ctx_.blockView, x, y + 1, z - 1);
  const float w = opacityAo(ctx_.blockView, x, y + 1, z + 1);
  const float face = opacityAo(ctx_.blockView, x, y + 1, z);
  const float ne = topNorthEdgeTranslucent || topWestEdgeTranslucent ? opacityAo(ctx_.blockView, x - 1, y + 1, z - 1)
                                                                     : n;
  const float se = topNorthEdgeTranslucent || topEastEdgeTranslucent ? opacityAo(ctx_.blockView, x + 1, y + 1, z - 1)
                                                                     : s;
  const float nw = topSouthEdgeTranslucent || topWestEdgeTranslucent ? opacityAo(ctx_.blockView, x - 1, y + 1, z + 1)
                                                                     : n;
  const float sw = topSouthEdgeTranslucent || topEastEdgeTranslucent ? opacityAo(ctx_.blockView, x + 1, y + 1, z + 1)
                                                                     : s;
  corner3 = (nw + n + w + face) / 4.0f;
  corner0 = (w + face + sw + s) / 4.0f;
  corner1 = (face + e + s + se) / 4.0f;
  corner2 = (n + ne + face + e) / 4.0f;
  averageCornerLight(ctx_, x, y + 1, z + 1, x, y + 1, z, x + 1, y + 1, z + 1, x + 1, y + 1, z, lb0, ls0);
  averageCornerLight(ctx_, x, y + 1, z, x, y + 1, z - 1, x + 1, y + 1, z, x + 1, y + 1, z - 1, lb1, ls1);
  averageCornerLight(ctx_, x - 1, y + 1, z, x - 1, y + 1, z - 1, x, y + 1, z, x, y + 1, z - 1, lb2, ls2);
  averageCornerLight(ctx_, x - 1, y + 1, z + 1, x - 1, y + 1, z, x, y + 1, z + 1, x, y + 1, z, lb3, ls3);
  assignAoCorners(ctx_.opts, ctx_.faceState, 1, corner0, corner1, corner2, corner3, tintUp, red, green, blue, lb0, ls0,
                  lb1, ls1, lb2, ls2, lb3, ls3);
  faces_.renderTopFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 1));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z - 1, 2)) {
  const float n = opacityAo(ctx_.blockView, x - 1, y, z - 1);
  const float bot = opacityAo(ctx_.blockView, x, y - 1, z - 1);
  const float top = opacityAo(ctx_.blockView, x, y + 1, z - 1);
  const float s = opacityAo(ctx_.blockView, x + 1, y, z - 1);
  const float face = opacityAo(ctx_.blockView, x, y, z - 1);
  const float nb = northWestEdgeTranslucent || bottomNorthEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x - 1, y - 1, z - 1)
                       : n;
  const float nt = northWestEdgeTranslucent || topNorthEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x - 1, y + 1, z - 1)
                       : n;
  const float sb = northEastEdgeTranslucent || bottomNorthEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x + 1, y - 1, z - 1)
                       : s;
  const float st = northEastEdgeTranslucent || topNorthEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x + 1, y + 1, z - 1)
                       : s;
  corner0 = (n + nt + face + top) / 4.0f;
  corner1 = (face + top + s + st) / 4.0f;
  corner2 = (bot + face + sb + s) / 4.0f;
  corner3 = (nb + n + bot + face) / 4.0f;
  averageCornerLight(ctx_, x - 1, y, z - 1, x - 1, y + 1, z - 1, x, y, z - 1, x, y + 1, z - 1, lb0, ls0);
  averageCornerLight(ctx_, x, y, z - 1, x, y + 1, z - 1, x + 1, y, z - 1, x + 1, y + 1, z - 1, lb1, ls1);
  averageCornerLight(ctx_, x, y - 1, z - 1, x, y, z - 1, x + 1, y - 1, z - 1, x + 1, y, z - 1, lb2, ls2);
  averageCornerLight(ctx_, x - 1, y - 1, z - 1, x - 1, y, z - 1, x, y - 1, z - 1, x, y, z - 1, lb3, ls3);
  assignAoCorners(ctx_.opts, ctx_.faceState, 2, corner0, corner1, corner2, corner3, tintEast, red, green, blue, lb0,
                  ls0, lb1, ls1, lb2, ls2, lb3, ls3);
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 2);
  faces_.renderEastFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   multiplyAoVertexColors(ctx_.faceState, red, green, blue);
   faces_.renderEastFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z + 1, 3)) {
  const float n = opacityAo(ctx_.blockView, x - 1, y, z + 1);
  const float s = opacityAo(ctx_.blockView, x + 1, y, z + 1);
  const float bot = opacityAo(ctx_.blockView, x, y - 1, z + 1);
  const float top = opacityAo(ctx_.blockView, x, y + 1, z + 1);
  const float face = opacityAo(ctx_.blockView, x, y, z + 1);
  const float nb = southWestEdgeTranslucent || bottomSouthEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x - 1, y - 1, z + 1)
                       : n;
  const float nt = southWestEdgeTranslucent || topSouthEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x - 1, y + 1, z + 1)
                       : n;
  const float sb = southEastEdgeTranslucent || bottomSouthEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x + 1, y - 1, z + 1)
                       : s;
  const float st = southEastEdgeTranslucent || topSouthEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x + 1, y + 1, z + 1)
                       : s;
  corner0 = (n + nt + face + top) / 4.0f;
  corner3 = (face + top + s + st) / 4.0f;
  corner2 = (bot + face + sb + s) / 4.0f;
  corner1 = (nb + n + bot + face) / 4.0f;
  averageCornerLight(ctx_, x - 1, y, z + 1, x - 1, y + 1, z + 1, x, y, z + 1, x, y + 1, z + 1, lb0, ls0);
  averageCornerLight(ctx_, x - 1, y - 1, z + 1, x - 1, y, z + 1, x, y - 1, z + 1, x, y, z + 1, lb1, ls1);
  averageCornerLight(ctx_, x, y - 1, z + 1, x, y, z + 1, x + 1, y - 1, z + 1, x + 1, y, z + 1, lb2, ls2);
  averageCornerLight(ctx_, x, y, z + 1, x, y + 1, z + 1, x + 1, y, z + 1, x + 1, y + 1, z + 1, lb3, ls3);
  assignAoCorners(ctx_.opts, ctx_.faceState, 3, corner0, corner1, corner2, corner3, tintWest, red, green, blue, lb0,
                  ls0, lb1, ls1, lb2, ls2, lb3, ls3);
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 3);
  faces_.renderWestFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   multiplyAoVertexColors(ctx_.faceState, red, green, blue);
   faces_.renderWestFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x - 1, y, z, 4)) {
  const float bot = opacityAo(ctx_.blockView, x - 1, y - 1, z);
  const float e = opacityAo(ctx_.blockView, x - 1, y, z - 1);
  const float w = opacityAo(ctx_.blockView, x - 1, y, z + 1);
  const float top = opacityAo(ctx_.blockView, x - 1, y + 1, z);
  const float face = opacityAo(ctx_.blockView, x - 1, y, z);
  const float eb = northWestEdgeTranslucent || bottomWestEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x - 1, y - 1, z - 1)
                       : e;
  const float wb = southWestEdgeTranslucent || bottomWestEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x - 1, y - 1, z + 1)
                       : w;
  const float et = northWestEdgeTranslucent || topWestEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x - 1, y + 1, z - 1)
                       : e;
  const float wt = southWestEdgeTranslucent || topWestEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x - 1, y + 1, z + 1)
                       : w;
  corner3 = (bot + wb + face + w) / 4.0f;
  corner0 = (face + w + top + wt) / 4.0f;
  corner1 = (e + face + et + top) / 4.0f;
  corner2 = (eb + bot + e + face) / 4.0f;
  averageCornerLight(ctx_, x - 1, y, z + 1, x - 1, y + 1, z + 1, x - 1, y, z, x - 1, y + 1, z, lb0, ls0);
  averageCornerLight(ctx_, x - 1, y, z - 1, x - 1, y, z, x - 1, y + 1, z - 1, x - 1, y + 1, z, lb1, ls1);
  averageCornerLight(ctx_, x - 1, y - 1, z - 1, x - 1, y - 1, z, x - 1, y, z - 1, x - 1, y, z, lb2, ls2);
  averageCornerLight(ctx_, x - 1, y - 1, z, x - 1, y - 1, z + 1, x - 1, y, z, x - 1, y, z + 1, lb3, ls3);
  assignAoCorners(ctx_.opts, ctx_.faceState, 4, corner0, corner1, corner2, corner3, tintNorth, red, green, blue, lb0,
                  ls0, lb1, ls1, lb2, ls2, lb3, ls3);
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 4);
  faces_.renderNorthFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   multiplyAoVertexColors(ctx_.faceState, red, green, blue);
   faces_.renderNorthFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x + 1, y, z, 5)) {
  const float bot = opacityAo(ctx_.blockView, x + 1, y - 1, z);
  const float e = opacityAo(ctx_.blockView, x + 1, y, z - 1);
  const float w = opacityAo(ctx_.blockView, x + 1, y, z + 1);
  const float top = opacityAo(ctx_.blockView, x + 1, y + 1, z);
  const float face = opacityAo(ctx_.blockView, x + 1, y, z);
  const float eb = bottomEastEdgeTranslucent || northEastEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x + 1, y - 1, z - 1)
                       : e;
  const float wb = bottomEastEdgeTranslucent || southEastEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x + 1, y - 1, z + 1)
                       : w;
  const float et = topEastEdgeTranslucent || northEastEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x + 1, y + 1, z - 1)
                       : e;
  const float wt = topEastEdgeTranslucent || southEastEdgeTranslucent
                       ? opacityAo(ctx_.blockView, x + 1, y + 1, z + 1)
                       : w;
  corner0 = (bot + wb + face + w) / 4.0f;
  corner3 = (face + w + top + wt) / 4.0f;
  corner2 = (e + face + et + top) / 4.0f;
  corner1 = (eb + bot + e + face) / 4.0f;
  averageCornerLight(ctx_, x + 1, y - 1, z, x + 1, y - 1, z + 1, x + 1, y, z, x + 1, y, z + 1, lb0, ls0);
  averageCornerLight(ctx_, x + 1, y - 1, z - 1, x + 1, y - 1, z, x + 1, y, z - 1, x + 1, y, z, lb1, ls1);
  averageCornerLight(ctx_, x + 1, y, z - 1, x + 1, y, z, x + 1, y + 1, z - 1, x + 1, y + 1, z, lb2, ls2);
  averageCornerLight(ctx_, x + 1, y, z, x + 1, y, z + 1, x + 1, y + 1, z, x + 1, y + 1, z + 1, lb3, ls3);
  assignAoCorners(ctx_.opts, ctx_.faceState, 5, corner0, corner1, corner2, corner3, tintSouth, red, green, blue, lb0,
                  ls0, lb1, ls1, lb2, ls2, lb3, ls3);
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 5);
  faces_.renderSouthFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   multiplyAoVertexColors(ctx_.faceState, red, green, blue);
   faces_.renderSouthFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 ctx_.faceState.useAo = false;
 return drewAnyFace;
}
// Flat lighting: tint only in vertex colour; absolute light in the lightmap.
// Face orientation shade is pack-owned (vanilla gbuffers).
bool CubeBlockRenderer::renderFlat(
    net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue) {
 if(ctx_.blockView == nullptr) {
  return false;
 }
 ctx_.faceState.useAo = false;
 Tessellator& tessellator = *ctx_.tess;
 bool drewAnyFace = false;
 int textureId = 0;
 float downRed = red;
 float downGreen = green;
 float downBlue = blue;
 float upRed = red;
 float upGreen = green;
 float upBlue = blue;
 float horizRed = red;
 float horizGreen = green;
 float horizBlue = blue;
 float nsRed = red;
 float nsGreen = green;
 float nsBlue = blue;
 if(net::minecraft::block::Block::GRASS_BLOCK != nullptr && &block == net::minecraft::block::Block::GRASS_BLOCK) {
  downRed = 1.0f;
  downGreen = 1.0f;
  downBlue = 1.0f;
  horizRed = 1.0f;
  horizGreen = 1.0f;
  horizBlue = 1.0f;
  nsRed = 1.0f;
  nsGreen = 1.0f;
  nsBlue = 1.0f;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y - 1, z, 0)) {
  ctx_.sampleFaceLight(x, y - 1, z);
  tessellator.color(flatFaceColor(ctx_.opts, 0, downRed), flatFaceColor(ctx_.opts, 0, downGreen),
                    flatFaceColor(ctx_.opts, 0, downBlue));
  faces_.renderBottomFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 0));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y + 1, z, 1)) {
  const bool self = ctx_.renderBounds.maxY != 1.0 && !block.material.isFluid();
  ctx_.sampleFaceLight(x, self ? y : y + 1, z);
  tessellator.color(flatFaceColor(ctx_.opts, 1, upRed), flatFaceColor(ctx_.opts, 1, upGreen),
                    flatFaceColor(ctx_.opts, 1, upBlue));
  faces_.renderTopFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 1));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z - 1, 2)) {
  ctx_.sampleFaceLight(x, y, ctx_.renderBounds.minZ > 0.0 ? z : z - 1);
  tessellator.color(flatFaceColor(ctx_.opts, 2, horizRed), flatFaceColor(ctx_.opts, 2, horizGreen),
                    flatFaceColor(ctx_.opts, 2, horizBlue));
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 2);
  faces_.renderEastFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   tessellator.color(flatFaceColor(ctx_.opts, 2, horizRed * red), flatFaceColor(ctx_.opts, 2, horizGreen * green),
                     flatFaceColor(ctx_.opts, 2, horizBlue * blue));
   faces_.renderEastFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z + 1, 3)) {
  ctx_.sampleFaceLight(x, y, ctx_.renderBounds.maxZ < 1.0 ? z : z + 1);
  tessellator.color(flatFaceColor(ctx_.opts, 3, horizRed), flatFaceColor(ctx_.opts, 3, horizGreen),
                    flatFaceColor(ctx_.opts, 3, horizBlue));
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 3);
  faces_.renderWestFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   tessellator.color(flatFaceColor(ctx_.opts, 3, horizRed * red), flatFaceColor(ctx_.opts, 3, horizGreen * green),
                     flatFaceColor(ctx_.opts, 3, horizBlue * blue));
   faces_.renderWestFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x - 1, y, z, 4)) {
  ctx_.sampleFaceLight(ctx_.renderBounds.minX > 0.0 ? x : x - 1, y, z);
  tessellator.color(flatFaceColor(ctx_.opts, 4, nsRed), flatFaceColor(ctx_.opts, 4, nsGreen),
                    flatFaceColor(ctx_.opts, 4, nsBlue));
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 4);
  faces_.renderNorthFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   tessellator.color(flatFaceColor(ctx_.opts, 4, nsRed * red), flatFaceColor(ctx_.opts, 4, nsGreen * green),
                     flatFaceColor(ctx_.opts, 4, nsBlue * blue));
   faces_.renderNorthFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x + 1, y, z, 5)) {
  ctx_.sampleFaceLight(ctx_.renderBounds.maxX < 1.0 ? x : x + 1, y, z);
  tessellator.color(flatFaceColor(ctx_.opts, 5, nsRed), flatFaceColor(ctx_.opts, 5, nsGreen),
                    flatFaceColor(ctx_.opts, 5, nsBlue));
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 5);
  faces_.renderSouthFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   tessellator.color(flatFaceColor(ctx_.opts, 5, nsRed * red), flatFaceColor(ctx_.opts, 5, nsGreen * green),
                     flatFaceColor(ctx_.opts, 5, nsBlue * blue));
   faces_.renderSouthFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 return drewAnyFace;
}
bool CubeBlockRenderer::renderCactus(net::minecraft::block::Block& block, int x, int y, int z) {
 int n = block.getColorMultiplier(ctx_.blockView, x, y, z);
 float red = (float)(n >> 16 & 0xFF) / 255.0f;
 float green = (float)(n >> 8 & 0xFF) / 255.0f;
 float blue = (float)(n & 0xFF) / 255.0f;
 return renderCactus(block, x, y, z, red, green, blue);
}
bool CubeBlockRenderer::renderCactus(
    net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue) {
 ctx_.faceState.useAo = false;
 Tessellator& tessellator = *ctx_.tess;
 bool drewAnyFace = false;
 const float inset = 0.0625f;
 const auto savedBounds = ctx_.renderBounds;
 ctx_.renderBounds = {inset, 0.0, inset, 1.0f - inset, 1.0, 1.0f - inset};
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y - 1, z, 0)) {
  ctx_.sampleFaceLight(x, y - 1, z);
  tessellator.color(flatFaceColor(ctx_.opts, 0, red), flatFaceColor(ctx_.opts, 0, green),
                    flatFaceColor(ctx_.opts, 0, blue));
  faces_.renderBottomFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 0));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y + 1, z, 1)) {
  ctx_.sampleFaceLight(ctx_.renderBounds.maxY < 1.0 && !block.material.isFluid() ? x : x, y + 1, z);
  tessellator.color(flatFaceColor(ctx_.opts, 1, red), flatFaceColor(ctx_.opts, 1, green),
                    flatFaceColor(ctx_.opts, 1, blue));
  faces_.renderTopFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 1));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z - 1, 2)) {
  ctx_.sampleFaceLight(x, y, ctx_.renderBounds.minZ > 0.0 ? z : z - 1);
  tessellator.color(flatFaceColor(ctx_.opts, 2, red), flatFaceColor(ctx_.opts, 2, green),
                    flatFaceColor(ctx_.opts, 2, blue));
  faces_.renderEastFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 2));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z + 1, 3)) {
  ctx_.sampleFaceLight(x, y, ctx_.renderBounds.maxZ < 1.0 ? z : z + 1);
  tessellator.color(flatFaceColor(ctx_.opts, 3, red), flatFaceColor(ctx_.opts, 3, green),
                    flatFaceColor(ctx_.opts, 3, blue));
  faces_.renderWestFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 3));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x - 1, y, z, 4)) {
  ctx_.sampleFaceLight(ctx_.renderBounds.minX > 0.0 ? x : x - 1, y, z);
  tessellator.color(flatFaceColor(ctx_.opts, 4, red), flatFaceColor(ctx_.opts, 4, green),
                    flatFaceColor(ctx_.opts, 4, blue));
  faces_.renderNorthFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 4));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x + 1, y, z, 5)) {
  ctx_.sampleFaceLight(ctx_.renderBounds.maxX < 1.0 ? x : x + 1, y, z);
  tessellator.color(flatFaceColor(ctx_.opts, 5, red), flatFaceColor(ctx_.opts, 5, green),
                    flatFaceColor(ctx_.opts, 5, blue));
  faces_.renderSouthFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 5));
  drewAnyFace = true;
 }
 ctx_.renderBounds = savedBounds;
 return drewAnyFace;
}
} // namespace net::minecraft::client::render::block
