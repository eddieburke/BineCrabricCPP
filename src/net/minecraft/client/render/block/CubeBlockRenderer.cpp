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
bool edgeTransmitsLight(const net::minecraft::BlockView* blockView, int x, int y, int z) {
 if(blockView == nullptr) {
  return true;
 }
 const int blockId = blockView->getBlockId(x, y, z);
 if(blockId <= 0 || blockId >= net::minecraft::block::Block::BLOCK_COUNT) {
  return true;
 }
 return net::minecraft::block::Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(blockId)] == 0;
}
void readCornerLight(BlockRenderContext& ctx, int x, int y, int z, int& blockLight, int& skyLight) {
 ctx.sampleFaceLight(x, y, z);
 blockLight = ctx.faceBlockLight;
 skyLight = ctx.faceSkyLight;
}
float cornerOcclusion(const net::minecraft::BlockView* blockView, int x, int y, int z) {
 if(blockView == nullptr) {
  return 1.0f;
 }
 const int blockId = blockView->getBlockId(x, y, z);
 if(blockId <= 0 || blockId >= net::minecraft::block::Block::BLOCK_COUNT) {
  return 1.0f;
 }
 return net::minecraft::block::Block::BLOCKS_OPAQUE[static_cast<std::size_t>(blockId)] ? 0.2f : 1.0f;
}
struct CornerSample {
 float blockLight = 15.0f;
 float skyLight = 15.0f;
 float occlusion = 1.0f;
};
float averageCornerChannel(int diagonal, int side, int other, int center) {
 if(diagonal == 0) diagonal = center;
 if(side == 0) side = center;
 if(other == 0) other = center;
 return static_cast<float>(diagonal + side + other + center) / 4.0f;
}
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
                        CornerSample& corner) {
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
 float od = cornerOcclusion(ctx.blockView, dx, dy, dz);
 const float os = cornerOcclusion(ctx.blockView, sx, sy, sz);
 const float oo = cornerOcclusion(ctx.blockView, ox, oy, oz);
 const float oc = cornerOcclusion(ctx.blockView, cx, cy, cz);
 if(closed) {
  bd = bs;
  sd = ss;
  // The diagonal is unreachable behind a closed inner corner, so vanilla reads
  // the edge in its place — for the occlusion average exactly as for the light.
  od = os;
 }
 corner.blockLight = averageCornerChannel(bd, bs, bo, bc);
 corner.skyLight = averageCornerChannel(sd, ss, so, sc);
 corner.occlusion = (od + os + oo + oc) / 4.0f;
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
float flatFaceColor(const option::RenderSettings& resolved, int face, float channel) {
 return resolved.oldLighting ? channel * oldLightingFaceShade(face) : channel;
}
void assignAoCorners(const option::RenderSettings& resolved,
                     BlockFaceRenderState& state,
                     int face,
                     bool applyTint,
                     float red,
                     float green,
                     float blue,
                     int fBlock,
                     int fSky,
                     const CornerSample (&corners)[4]) {
 const float tintRed = applyTint ? red : 1.0f;
 const float tintGreen = applyTint ? green : 1.0f;
 const float tintBlue = applyTint ? blue : 1.0f;
 const float shade = resolved.oldLighting ? oldLightingFaceShade(face) : 1.0f;
 const float strength = resolved.ambientOcclusionStrength;
 for(int i = 0; i < 4; ++i) {
  const float ao = 1.0f - (1.0f - corners[i].occlusion) * strength;
  const float coefficient = ao * shade;
  const float rgbCoefficient = resolved.separateAo ? 1.0f : coefficient;
  state.colors.red[i] = tintRed * rgbCoefficient;
  state.colors.green[i] = tintGreen * rgbCoefficient;
  state.colors.blue[i] = tintBlue * rgbCoefficient;
  state.colors.alpha[i] = resolved.separateAo ? coefficient : 1.0f;
  state.blockLight[i] =
      static_cast<float>(fBlock) + (corners[i].blockLight - static_cast<float>(fBlock)) * strength;
  state.skyLight[i] = static_cast<float>(fSky) + (corners[i].skyLight - static_cast<float>(fSky)) * strength;
 }
}
void multiplyVertexColors(BlockFaceRenderState& state, float red, float green, float blue) {
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
bool CubeBlockRenderer::renderSmooth(
    net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue) {
 if(ctx_.blockView == nullptr) {
  return false;
 }
 int textureId = 0;
 ctx_.faceState.useAo = true;
 bool drewAnyFace = false;
 CornerSample corners[4]{};
 bool tintDown = true;
 bool tintUp = true;
 bool tintEast = true;
 bool tintWest = true;
 bool tintNorth = true;
 bool tintSouth = true;
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
  const int faceY = ctx_.renderBounds.minY > 0.0 ? y : y - 1;
  const bool westEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x - 1, faceY, z);
  const bool eastEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x + 1, faceY, z);
  const bool southEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x, faceY, z + 1);
  const bool northEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x, faceY, z - 1);
  ctx_.sampleFaceLight(x, faceY, z);
  const int fBlock = ctx_.faceBlockLight;
  const int fSky = ctx_.faceSkyLight;
  averageCornerLight(ctx_, x - 1, faceY, z + 1, x - 1, faceY, z, x, faceY, z + 1, x, faceY, z,
                     !(westEdgeTranslucent || southEdgeTranslucent), corners[0]);
  averageCornerLight(ctx_, x - 1, faceY, z - 1, x - 1, faceY, z, x, faceY, z - 1, x, faceY, z,
                     !(westEdgeTranslucent || northEdgeTranslucent), corners[1]);
  averageCornerLight(ctx_, x + 1, faceY, z - 1, x + 1, faceY, z, x, faceY, z - 1, x, faceY, z,
                     !(eastEdgeTranslucent || northEdgeTranslucent), corners[2]);
  averageCornerLight(ctx_, x + 1, faceY, z + 1, x + 1, faceY, z, x, faceY, z + 1, x, faceY, z,
                     !(eastEdgeTranslucent || southEdgeTranslucent), corners[3]);
  assignAoCorners(ctx_.opts, ctx_.faceState, 0, tintDown, red, green, blue, fBlock, fSky, corners);
  faces_.renderBottomFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 0));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y + 1, z, 1)) {
  const int faceY = ctx_.renderBounds.maxY < 1.0 && !block.material.isFluid() ? y : y + 1;
  const bool eastEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x + 1, faceY, z);
  const bool westEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x - 1, faceY, z);
  const bool southEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x, faceY, z + 1);
  const bool northEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x, faceY, z - 1);
  ctx_.sampleFaceLight(x, faceY, z);
  const int fBlock = ctx_.faceBlockLight;
  const int fSky = ctx_.faceSkyLight;
  averageCornerLight(ctx_, x + 1, faceY, z + 1, x + 1, faceY, z, x, faceY, z + 1, x, faceY, z,
                     !(eastEdgeTranslucent || southEdgeTranslucent), corners[0]);
  averageCornerLight(ctx_, x + 1, faceY, z - 1, x + 1, faceY, z, x, faceY, z - 1, x, faceY, z,
                     !(eastEdgeTranslucent || northEdgeTranslucent), corners[1]);
  averageCornerLight(ctx_, x - 1, faceY, z - 1, x - 1, faceY, z, x, faceY, z - 1, x, faceY, z,
                     !(westEdgeTranslucent || northEdgeTranslucent), corners[2]);
  averageCornerLight(ctx_, x - 1, faceY, z + 1, x - 1, faceY, z, x, faceY, z + 1, x, faceY, z,
                     !(westEdgeTranslucent || southEdgeTranslucent), corners[3]);
  assignAoCorners(ctx_.opts, ctx_.faceState, 1, tintUp, red, green, blue, fBlock, fSky, corners);
  faces_.renderTopFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 1));
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z - 1, 2)) {
  const int faceZ = ctx_.renderBounds.minZ > 0.0 ? z : z - 1;
  const bool westEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x - 1, y, faceZ);
  const bool eastEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x + 1, y, faceZ);
  const bool topEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x, y + 1, faceZ);
  const bool bottomEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x, y - 1, faceZ);
  ctx_.sampleFaceLight(x, y, faceZ);
  const int fBlock = ctx_.faceBlockLight;
  const int fSky = ctx_.faceSkyLight;
  averageCornerLight(ctx_, x - 1, y + 1, faceZ, x - 1, y, faceZ, x, y + 1, faceZ, x, y, faceZ,
                     !(westEdgeTranslucent || topEdgeTranslucent), corners[0]);
  averageCornerLight(ctx_, x + 1, y + 1, faceZ, x + 1, y, faceZ, x, y + 1, faceZ, x, y, faceZ,
                     !(eastEdgeTranslucent || topEdgeTranslucent), corners[1]);
  averageCornerLight(ctx_, x + 1, y - 1, faceZ, x + 1, y, faceZ, x, y - 1, faceZ, x, y, faceZ,
                     !(eastEdgeTranslucent || bottomEdgeTranslucent), corners[2]);
  averageCornerLight(ctx_, x - 1, y - 1, faceZ, x - 1, y, faceZ, x, y - 1, faceZ, x, y, faceZ,
                     !(westEdgeTranslucent || bottomEdgeTranslucent), corners[3]);
  assignAoCorners(ctx_.opts, ctx_.faceState, 2, tintEast, red, green, blue, fBlock, fSky, corners);
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 2);
  faces_.renderEastFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   multiplyVertexColors(ctx_.faceState, red, green, blue);
   faces_.renderEastFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z + 1, 3)) {
  const int faceZ = ctx_.renderBounds.maxZ < 1.0 ? z : z + 1;
  const bool westEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x - 1, y, faceZ);
  const bool eastEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x + 1, y, faceZ);
  const bool topEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x, y + 1, faceZ);
  const bool bottomEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, x, y - 1, faceZ);
  ctx_.sampleFaceLight(x, y, faceZ);
  const int fBlock = ctx_.faceBlockLight;
  const int fSky = ctx_.faceSkyLight;
  averageCornerLight(ctx_, x - 1, y + 1, faceZ, x - 1, y, faceZ, x, y + 1, faceZ, x, y, faceZ,
                     !(westEdgeTranslucent || topEdgeTranslucent), corners[0]);
  averageCornerLight(ctx_, x - 1, y - 1, faceZ, x - 1, y, faceZ, x, y - 1, faceZ, x, y, faceZ,
                     !(westEdgeTranslucent || bottomEdgeTranslucent), corners[1]);
  averageCornerLight(ctx_, x + 1, y - 1, faceZ, x + 1, y, faceZ, x, y - 1, faceZ, x, y, faceZ,
                     !(eastEdgeTranslucent || bottomEdgeTranslucent), corners[2]);
  averageCornerLight(ctx_, x + 1, y + 1, faceZ, x + 1, y, faceZ, x, y + 1, faceZ, x, y, faceZ,
                     !(eastEdgeTranslucent || topEdgeTranslucent), corners[3]);
  assignAoCorners(ctx_.opts, ctx_.faceState, 3, tintWest, red, green, blue, fBlock, fSky, corners);
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 3);
  faces_.renderWestFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   multiplyVertexColors(ctx_.faceState, red, green, blue);
   faces_.renderWestFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x - 1, y, z, 4)) {
  const int faceX = ctx_.renderBounds.minX > 0.0 ? x : x - 1;
  const bool southEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, faceX, y, z + 1);
  const bool northEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, faceX, y, z - 1);
  const bool topEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, faceX, y + 1, z);
  const bool bottomEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, faceX, y - 1, z);
  ctx_.sampleFaceLight(faceX, y, z);
  const int fBlock = ctx_.faceBlockLight;
  const int fSky = ctx_.faceSkyLight;
  averageCornerLight(ctx_, faceX, y + 1, z + 1, faceX, y, z + 1, faceX, y + 1, z, faceX, y, z,
                     !(southEdgeTranslucent || topEdgeTranslucent), corners[0]);
  averageCornerLight(ctx_, faceX, y + 1, z - 1, faceX, y, z - 1, faceX, y + 1, z, faceX, y, z,
                     !(northEdgeTranslucent || topEdgeTranslucent), corners[1]);
  averageCornerLight(ctx_, faceX, y - 1, z - 1, faceX, y, z - 1, faceX, y - 1, z, faceX, y, z,
                     !(northEdgeTranslucent || bottomEdgeTranslucent), corners[2]);
  averageCornerLight(ctx_, faceX, y - 1, z + 1, faceX, y, z + 1, faceX, y - 1, z, faceX, y, z,
                     !(southEdgeTranslucent || bottomEdgeTranslucent), corners[3]);
  assignAoCorners(ctx_.opts, ctx_.faceState, 4, tintNorth, red, green, blue, fBlock, fSky, corners);
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 4);
  faces_.renderNorthFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   multiplyVertexColors(ctx_.faceState, red, green, blue);
   faces_.renderNorthFace(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 if(ctx_.skipFaceCulling || ctx_.isSideVisible(block, x + 1, y, z, 5)) {
  const int faceX = ctx_.renderBounds.maxX < 1.0 ? x : x + 1;
  const bool southEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, faceX, y, z + 1);
  const bool northEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, faceX, y, z - 1);
  const bool topEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, faceX, y + 1, z);
  const bool bottomEdgeTranslucent = edgeTransmitsLight(ctx_.blockView, faceX, y - 1, z);
  ctx_.sampleFaceLight(faceX, y, z);
  const int fBlock = ctx_.faceBlockLight;
  const int fSky = ctx_.faceSkyLight;
  averageCornerLight(ctx_, faceX, y - 1, z + 1, faceX, y, z + 1, faceX, y - 1, z, faceX, y, z,
                     !(bottomEdgeTranslucent || southEdgeTranslucent), corners[0]);
  averageCornerLight(ctx_, faceX, y - 1, z - 1, faceX, y, z - 1, faceX, y - 1, z, faceX, y, z,
                     !(bottomEdgeTranslucent || northEdgeTranslucent), corners[1]);
  averageCornerLight(ctx_, faceX, y + 1, z - 1, faceX, y, z - 1, faceX, y + 1, z, faceX, y, z,
                     !(topEdgeTranslucent || northEdgeTranslucent), corners[2]);
  averageCornerLight(ctx_, faceX, y + 1, z + 1, faceX, y, z + 1, faceX, y + 1, z, faceX, y, z,
                     !(topEdgeTranslucent || southEdgeTranslucent), corners[3]);
  assignAoCorners(ctx_.opts, ctx_.faceState, 5, tintSouth, red, green, blue, fBlock, fSky, corners);
  textureId = block.getTextureId(ctx_.blockView, x, y, z, 5);
  faces_.renderSouthFace(block, x, y, z, textureId);
  if(grassSideTintActive(ctx_.opts) && textureId == 3 && ctx_.textureOverride < 0) {
   multiplyVertexColors(ctx_.faceState, red, green, blue);
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
 Tessellator& tessellator = ctx_.tessellator();
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
 Tessellator& tessellator = ctx_.tessellator();
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
