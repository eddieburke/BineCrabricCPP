#include <algorithm>
#include <cmath>
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
void readCornerLight(BlockRenderContext& ctx, int x, int y, int z, int& blockLight, int& skyLight) {
 ctx.sampleFaceLight(x, y, z);
 blockLight = ctx.faceBlockLight;
 skyLight = ctx.faceSkyLight;
}
// Vanilla b1.7.3 corner average: four light samples (diagonal, x-edge, z-edge,
// centre) with the diagonal dropped for a closed inner corner (both edges
// opaque) so concave geometry keeps the classic AO crevice darkening. The
// lightmap carries this per-corner light; the vertex AO colour term stays
// neutral so the lightmap is not multiplied a second time (that double-dark
// shading is what made smooth lighting look pitch black).
//
// The average stays fractional. Rounding it to a whole level used to fold
// {15,15,15,14}, {15,15,14,14} and {15,14,14,14} all onto level 14, and the
// luminance curve drops 21% between 15 and 14 — so a face had nothing between
// "fully lit" and one hard step down, which read as banding rather than AO.
void averageCornerLight(BlockRenderContext& ctx,
                        int dx,
                        int dy,
                        int dz,
                        int sx,
                        int sy,
                        int sz,
                        int ox,
                        int oy,
                        int oz,
                        int cx,
                        int cy,
                        int cz,
                        bool closed,
                        float& blockLight,
                        float& skyLight) {
 int bd = 0;
 int sd = 0;
 int bs = 0;
 int ss = 0;
 int bo = 0;
 int so = 0;
 int bc = 0;
 int sc = 0;
 readCornerLight(ctx, dx, dy, dz, bd, sd);
 readCornerLight(ctx, sx, sy, sz, bs, ss);
 readCornerLight(ctx, ox, oy, oz, bo, so);
 readCornerLight(ctx, cx, cy, cz, bc, sc);
 if(closed) {
  bd = bs;
  sd = ss;
 }
 blockLight = static_cast<float>(bd + bs + bo + bc) / 4.0f;
 skyLight = static_cast<float>(sd + ss + so + sc) / 4.0f;
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
// Smooth-lighting corner assignment: the vertex colour carries only the tint
// (neutral AO — the lightmap is the single light source), and each vertex's
// lightmap coordinate blends the flat face-adjacent light toward the vanilla
// corner-averaged light by the AO strength slider. separateAo/oldLighting stay
// honoured for packs that expect them.
void assignAoCorners(const option::RenderSettings& resolved,
                     BlockFaceRenderState& state,
                     int face,
                     bool applyTint,
                     float red,
                     float green,
                     float blue,
                     int fBlock,
                     int fSky,
                     float b0,
                     float s0,
                     float b1,
                     float s1,
                     float b2,
                     float s2,
                     float b3,
                     float s3) {
 const float tintRed = applyTint ? red : 1.0f;
 const float tintGreen = applyTint ? green : 1.0f;
 const float tintBlue = applyTint ? blue : 1.0f;
 for(int i = 0; i < 4; ++i) {
  state.colors.red[i] = tintRed;
  state.colors.green[i] = tintGreen;
  state.colors.blue[i] = tintBlue;
  state.colors.alpha[i] = 1.0f;
 }
 const float cornerBlock[4] = {b0, b1, b2, b3};
 const float cornerSky[4] = {s0, s1, s2, s3};
 const float strength = resolved.ambientOcclusionStrength;
 // No rounding: the slider has to fade continuously, not snap a whole level at
 // a time, and the corner itself carries quarter-level detail.
 for(int i = 0; i < 4; ++i) {
  state.blockLight[i] = static_cast<float>(fBlock) + (cornerBlock[i] - static_cast<float>(fBlock)) * strength;
  state.skyLight[i] = static_cast<float>(fSky) + (cornerSky[i] - static_cast<float>(fSky)) * strength;
 }
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
// Smooth lighting: C++ writes vanilla corner-averaged light into per-vertex
// lightmap coordinates (vaUV2); the vertex colour stays tint-only so the light
// is applied exactly once. Packs own face shading (faceShade) and the lightmap
// sample; the AO strength slider blends the corners toward the flat face light.
bool CubeBlockRenderer::renderSmooth(
    net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue) {
 if(ctx_.blockView == nullptr) {
  return false;
 }
 int textureId = 0;
 ctx_.faceState.useAo = true;
 bool drewAnyFace = false;
 float lb0 = 15.0f;
 float ls0 = 15.0f;
 float lb1 = 15.0f;
 float ls1 = 15.0f;
 float lb2 = 15.0f;
 float ls2 = 15.0f;
 float lb3 = 15.0f;
 float ls3 = 15.0f;
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
   // Flat baseline: the face-adjacent light the AO slider blends toward.
   ctx_.sampleFaceLight(x, y - 1, z);
   const int fBlock = ctx_.faceBlockLight;
   const int fSky = ctx_.faceSkyLight;
   averageCornerLight(ctx_, x - 1, y - 1, z + 1, x - 1, y - 1, z, x, y - 1, z + 1, x, y - 1, z,
                      !(bottomWestEdgeTranslucent || bottomSouthEdgeTranslucent), lb0, ls0);
   averageCornerLight(ctx_, x - 1, y - 1, z - 1, x - 1, y - 1, z, x, y - 1, z - 1, x, y - 1, z,
                      !(bottomWestEdgeTranslucent || bottomNorthEdgeTranslucent), lb1, ls1);
   averageCornerLight(ctx_, x + 1, y - 1, z - 1, x + 1, y - 1, z, x, y - 1, z - 1, x, y - 1, z,
                      !(bottomEastEdgeTranslucent || bottomNorthEdgeTranslucent), lb2, ls2);
   averageCornerLight(ctx_, x + 1, y - 1, z + 1, x + 1, y - 1, z, x, y - 1, z + 1, x, y - 1, z,
                      !(bottomEastEdgeTranslucent || bottomSouthEdgeTranslucent), lb3, ls3);
   assignAoCorners(ctx_.opts, ctx_.faceState, 0, tintDown, red, green, blue, fBlock, fSky, lb0, ls0, lb1, ls1,
                   lb2, ls2, lb3, ls3);
   faces_.renderBottomFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 0));
   drewAnyFace = true;
  }
  if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y + 1, z, 1)) {
   const bool self = ctx_.renderBounds.maxY != 1.0 && !block.material.isFluid();
   ctx_.sampleFaceLight(x, self ? y : y + 1, z);
   const int fBlock = ctx_.faceBlockLight;
   const int fSky = ctx_.faceSkyLight;
   averageCornerLight(ctx_, x + 1, y + 1, z + 1, x + 1, y + 1, z, x, y + 1, z + 1, x, y + 1, z,
                      !(topEastEdgeTranslucent || topSouthEdgeTranslucent), lb0, ls0);
   averageCornerLight(ctx_, x + 1, y + 1, z - 1, x + 1, y + 1, z, x, y + 1, z - 1, x, y + 1, z,
                      !(topEastEdgeTranslucent || topNorthEdgeTranslucent), lb1, ls1);
   averageCornerLight(ctx_, x - 1, y + 1, z - 1, x - 1, y + 1, z, x, y + 1, z - 1, x, y + 1, z,
                      !(topWestEdgeTranslucent || topNorthEdgeTranslucent), lb2, ls2);
   averageCornerLight(ctx_, x - 1, y + 1, z + 1, x - 1, y + 1, z, x, y + 1, z + 1, x, y + 1, z,
                      !(topWestEdgeTranslucent || topSouthEdgeTranslucent), lb3, ls3);
   assignAoCorners(ctx_.opts, ctx_.faceState, 1, tintUp, red, green, blue, fBlock, fSky, lb0, ls0, lb1, ls1,
                   lb2, ls2, lb3, ls3);
   faces_.renderTopFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 1));
   drewAnyFace = true;
  }
  if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z - 1, 2)) {
   ctx_.sampleFaceLight(x, y, ctx_.renderBounds.minZ > 0.0 ? z : z - 1);
   const int fBlock = ctx_.faceBlockLight;
   const int fSky = ctx_.faceSkyLight;
   averageCornerLight(ctx_, x - 1, y + 1, z - 1, x - 1, y, z - 1, x, y + 1, z - 1, x, y, z - 1,
                      !(northWestEdgeTranslucent || topNorthEdgeTranslucent), lb0, ls0);
   averageCornerLight(ctx_, x + 1, y + 1, z - 1, x + 1, y, z - 1, x, y + 1, z - 1, x, y, z - 1,
                      !(northEastEdgeTranslucent || topNorthEdgeTranslucent), lb1, ls1);
   averageCornerLight(ctx_, x + 1, y - 1, z - 1, x + 1, y, z - 1, x, y - 1, z - 1, x, y, z - 1,
                      !(northEastEdgeTranslucent || bottomNorthEdgeTranslucent), lb2, ls2);
   averageCornerLight(ctx_, x - 1, y - 1, z - 1, x - 1, y, z - 1, x, y - 1, z - 1, x, y, z - 1,
                      !(northWestEdgeTranslucent || bottomNorthEdgeTranslucent), lb3, ls3);
   assignAoCorners(ctx_.opts, ctx_.faceState, 2, tintEast, red, green, blue, fBlock, fSky, lb0, ls0, lb1, ls1,
                   lb2, ls2, lb3, ls3);
   textureId = block.getTextureId(ctx_.blockView, x, y, z, 2);
   faces_.renderEastFace(block, x, y, z, textureId);
   if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
    multiplyAoVertexColors(ctx_.faceState, red, green, blue);
    faces_.renderEastFace(block, x, y, z, 38);
   }
   drewAnyFace = true;
  }
  if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z + 1, 3)) {
   ctx_.sampleFaceLight(x, y, ctx_.renderBounds.maxZ < 1.0 ? z : z + 1);
   const int fBlock = ctx_.faceBlockLight;
   const int fSky = ctx_.faceSkyLight;
   averageCornerLight(ctx_, x - 1, y + 1, z + 1, x - 1, y, z + 1, x, y + 1, z + 1, x, y, z + 1,
                      !(southWestEdgeTranslucent || topSouthEdgeTranslucent), lb0, ls0);
   averageCornerLight(ctx_, x - 1, y - 1, z + 1, x - 1, y, z + 1, x, y - 1, z + 1, x, y, z + 1,
                      !(southWestEdgeTranslucent || bottomSouthEdgeTranslucent), lb1, ls1);
   averageCornerLight(ctx_, x + 1, y - 1, z + 1, x + 1, y, z + 1, x, y - 1, z + 1, x, y, z + 1,
                      !(southEastEdgeTranslucent || bottomSouthEdgeTranslucent), lb2, ls2);
   averageCornerLight(ctx_, x + 1, y + 1, z + 1, x + 1, y, z + 1, x, y + 1, z + 1, x, y, z + 1,
                      !(southEastEdgeTranslucent || topSouthEdgeTranslucent), lb3, ls3);
   assignAoCorners(ctx_.opts, ctx_.faceState, 3, tintWest, red, green, blue, fBlock, fSky, lb0, ls0, lb1, ls1,
                   lb2, ls2, lb3, ls3);
   textureId = block.getTextureId(ctx_.blockView, x, y, z, 3);
   faces_.renderWestFace(block, x, y, z, textureId);
   if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
    multiplyAoVertexColors(ctx_.faceState, red, green, blue);
    faces_.renderWestFace(block, x, y, z, 38);
   }
   drewAnyFace = true;
  }
  if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x - 1, y, z, 4)) {
   ctx_.sampleFaceLight(ctx_.renderBounds.minX > 0.0 ? x : x - 1, y, z);
   const int fBlock = ctx_.faceBlockLight;
   const int fSky = ctx_.faceSkyLight;
   averageCornerLight(ctx_, x - 1, y + 1, z + 1, x - 1, y, z + 1, x - 1, y + 1, z, x - 1, y, z,
                      !(southWestEdgeTranslucent || topWestEdgeTranslucent), lb0, ls0);
   averageCornerLight(ctx_, x - 1, y + 1, z - 1, x - 1, y, z - 1, x - 1, y + 1, z, x - 1, y, z,
                      !(northWestEdgeTranslucent || topWestEdgeTranslucent), lb1, ls1);
   averageCornerLight(ctx_, x - 1, y - 1, z - 1, x - 1, y, z - 1, x - 1, y - 1, z, x - 1, y, z,
                      !(northWestEdgeTranslucent || bottomWestEdgeTranslucent), lb2, ls2);
   averageCornerLight(ctx_, x - 1, y - 1, z + 1, x - 1, y, z + 1, x - 1, y - 1, z, x - 1, y, z,
                      !(southWestEdgeTranslucent || bottomWestEdgeTranslucent), lb3, ls3);
   assignAoCorners(ctx_.opts, ctx_.faceState, 4, tintNorth, red, green, blue, fBlock, fSky, lb0, ls0, lb1, ls1,
                   lb2, ls2, lb3, ls3);
   textureId = block.getTextureId(ctx_.blockView, x, y, z, 4);
   faces_.renderNorthFace(block, x, y, z, textureId);
   if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
    multiplyAoVertexColors(ctx_.faceState, red, green, blue);
    faces_.renderNorthFace(block, x, y, z, 38);
   }
   drewAnyFace = true;
  }
  if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x + 1, y, z, 5)) {
   ctx_.sampleFaceLight(ctx_.renderBounds.maxX < 1.0 ? x : x + 1, y, z);
   const int fBlock = ctx_.faceBlockLight;
   const int fSky = ctx_.faceSkyLight;
   averageCornerLight(ctx_, x + 1, y - 1, z + 1, x + 1, y, z + 1, x + 1, y - 1, z, x + 1, y, z,
                      !(bottomEastEdgeTranslucent || southEastEdgeTranslucent), lb0, ls0);
   averageCornerLight(ctx_, x + 1, y - 1, z - 1, x + 1, y, z - 1, x + 1, y - 1, z, x + 1, y, z,
                      !(bottomEastEdgeTranslucent || northEastEdgeTranslucent), lb1, ls1);
   averageCornerLight(ctx_, x + 1, y + 1, z - 1, x + 1, y, z - 1, x + 1, y + 1, z, x + 1, y, z,
                      !(topEastEdgeTranslucent || northEastEdgeTranslucent), lb2, ls2);
   averageCornerLight(ctx_, x + 1, y + 1, z + 1, x + 1, y, z + 1, x + 1, y + 1, z, x + 1, y, z,
                      !(topEastEdgeTranslucent || southEastEdgeTranslucent), lb3, ls3);
   assignAoCorners(ctx_.opts, ctx_.faceState, 5, tintSouth, red, green, blue, fBlock, fSky, lb0, ls0, lb1, ls1,
                   lb2, ls2, lb3, ls3);
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
