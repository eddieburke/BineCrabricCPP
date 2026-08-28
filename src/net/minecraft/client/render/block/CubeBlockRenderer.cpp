#include <algorithm>
#include <cmath>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockFaceLighting.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/world/BlockView.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/WorldRegion.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightType.hpp"
namespace net::minecraft::client::render::block {
namespace option = net::minecraft::client::option;
namespace {
using FaceFn = void (BlockFaceRenderer::*)(net::minecraft::block::Block&, double, double, double, int);
constexpr FaceFn kFaceRenderers[6] = {
    &BlockFaceRenderer::renderBottomFace, &BlockFaceRenderer::renderTopFace,
    &BlockFaceRenderer::renderEastFace,   &BlockFaceRenderer::renderWestFace,
    &BlockFaceRenderer::renderNorthFace,  &BlockFaceRenderer::renderSouthFace,
};
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
// see third_party/iris/common/src/main/java/net/irisshaders/iris/mixin/MixinBlockStateBehavior.java:44
// ambientOcclusionLevel scales the occlusion coefficient and nothing else. Java
// reaches it through getShadeBrightness, which never touches the lightmap, so the
// per-corner smooth light stays smooth however far the slider is pulled down.
void assignAoCorners(const option::RenderSettings& resolved,
                     BlockFaceRenderState& state,
                     int face,
                     bool applyTint,
                     float red,
                     float green,
                     float blue,
                     const CornerSample (&corners)[4]) {
 const float tintRed = applyTint ? red : 1.0f;
 const float tintGreen = applyTint ? green : 1.0f;
 const float tintBlue = applyTint ? blue : 1.0f;
 const float shade = resolved.oldLighting ? oldLightingFaceShade(face) : 1.0f;
 const float strength = resolved.ambientOcclusionStrength;
 for(int i = 0; i < 4; ++i) {
  const float coefficient = (1.0f - (1.0f - corners[i].occlusion) * strength) * shade;
  const float rgbCoefficient = resolved.separateAo ? 1.0f : coefficient;
  state.colors.red[i] = tintRed * rgbCoefficient;
  state.colors.green[i] = tintGreen * rgbCoefficient;
  state.colors.blue[i] = tintBlue * rgbCoefficient;
  state.colors.alpha[i] = resolved.separateAo ? coefficient : 1.0f;
  state.blockLight[i] = corners[i].blockLight;
  state.skyLight[i] = corners[i].skyLight;
 }
}
void multiplyVertexColors(BlockFaceRenderState& state, float red, float green, float blue) {
 for(int i = 0; i < 4; ++i) {
  state.colors.red[i] *= red;
  state.colors.green[i] *= green;
  state.colors.blue[i] *= blue;
 }
}
struct FaceCell {
 int x;
 int y;
 int z;
};
// A partial block keeps its face on its own cell; only a face flush with the block
// boundary samples the neighbour. The top face additionally never pulls in for a
// fluid, whose surface sits below 1.0 but still lights as a full block.
FaceCell smoothFaceCell(const BlockRenderContext& ctx,
                        net::minecraft::block::Block& block,
                        int face,
                        int x,
                        int y,
                        int z) {
 switch(face) {
 case 0:
  return {x, ctx.renderBounds.minY > 0.0 ? y : y - 1, z};
 case 1:
  return {x, ctx.renderBounds.maxY < 1.0 && !block.material.isFluid() ? y : y + 1, z};
 case 2:
  return {x, y, ctx.renderBounds.minZ > 0.0 ? z : z - 1};
 case 3:
  return {x, y, ctx.renderBounds.maxZ < 1.0 ? z : z + 1};
 case 4:
  return {ctx.renderBounds.minX > 0.0 ? x : x - 1, y, z};
 default:
  return {ctx.renderBounds.maxX < 1.0 ? x : x + 1, y, z};
 }
}
// The unlit path samples its bottom face one cell down unconditionally and tests
// the top face's bound with != rather than <. Both differ from smoothFaceCell and
// both are load-bearing for slabs and stairs.
FaceCell flatFaceCell(const BlockRenderContext& ctx,
                      net::minecraft::block::Block& block,
                      int face,
                      int x,
                      int y,
                      int z) {
 switch(face) {
 case 0:
  return {x, y - 1, z};
 case 1:
  return {x, ctx.renderBounds.maxY != 1.0 && !block.material.isFluid() ? y : y + 1, z};
 default:
  return smoothFaceCell(ctx, block, face, x, y, z);
 }
}
// Cactus draws the same six unlit faces as renderFlat; its inset render bounds are
// the whole difference, and they collapse every side cell back onto the block's own.
bool renderUnlitFaces(BlockRenderContext& ctx,
                      BlockFaceRenderer& faces,
                      net::minecraft::block::Block& block,
                      int x,
                      int y,
                      int z,
                      float red,
                      float green,
                      float blue) {
 ctx.faceState.useAo = false;
 Tessellator& tessellator = ctx.tessellator();
 const bool grass = net::minecraft::block::Block::GRASS_BLOCK != nullptr &&
                    &block == net::minecraft::block::Block::GRASS_BLOCK;
 bool drewAnyFace = false;
 for(int face = 0; face < 6; ++face) {
  const int nx = x + kFaceOffsets[face][0];
  const int ny = y + kFaceOffsets[face][1];
  const int nz = z + kFaceOffsets[face][2];
  if(!ctx.skipFaceCulling && !ctx.isSideVisible(block, nx, ny, nz, face)) {
   continue;
  }
  const FaceCell cell = flatFaceCell(ctx, block, face, x, y, z);
  ctx.sampleFaceLight(cell.x, cell.y, cell.z);
  // Grass keeps its biome tint on the top face and draws the four sides white,
  // with the tinted overlay layered on top of them.
  const bool untinted = grass && face != 1;
  const float faceRed = untinted ? 1.0f : red;
  const float faceGreen = untinted ? 1.0f : green;
  const float faceBlue = untinted ? 1.0f : blue;
  tessellator.color(flatFaceColor(ctx.opts, face, faceRed), flatFaceColor(ctx.opts, face, faceGreen),
                    flatFaceColor(ctx.opts, face, faceBlue));
  const int textureId = block.getTextureId(ctx.blockView, x, y, z, face);
  (faces.*kFaceRenderers[face])(block, x, y, z, textureId);
  if(face >= 2 && ctx.opts.fancyGrass && textureId == 3 && ctx.textureOverride < 0) {
   tessellator.color(flatFaceColor(ctx.opts, face, faceRed * red),
                     flatFaceColor(ctx.opts, face, faceGreen * green),
                     flatFaceColor(ctx.opts, face, faceBlue * blue));
   (faces.*kFaceRenderers[face])(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 return drewAnyFace;
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
 ctx_.faceState.useAo = true;
 bool drewAnyFace = false;
 bool tint[6] = {true, true, true, true, true, true};
 if(block.textureId == 3 || ctx_.textureOverride >= 0) {
  tint[0] = false;
  tint[2] = false;
  tint[3] = false;
  tint[4] = false;
  tint[5] = false;
 }
 for(int face = 0; face < 6; ++face) {
  const int nx = x + kFaceOffsets[face][0];
  const int ny = y + kFaceOffsets[face][1];
  const int nz = z + kFaceOffsets[face][2];
  if(!ctx_.skipFaceCulling && !ctx_.isSideVisible(block, nx, ny, nz, face)) {
   continue;
  }
  const FaceCell cell = smoothFaceCell(ctx_, block, face, x, y, z);
  const FaceCornerSamples samples = sampleCubeFaceCorners(ctx_, face, cell.x, cell.y, cell.z);
  CornerSample corners[4]{};
  samples.toWinding(corners);
  assignAoCorners(ctx_.opts, ctx_.faceState, face, tint[face], red, green, blue, corners);
  const int textureId = block.getTextureId(ctx_.blockView, x, y, z, face);
  (faces_.*kFaceRenderers[face])(block, x, y, z, textureId);
  // Grass keeps its side overlay on the four side faces only.
  if(face >= 2 && ctx_.opts.fancyGrass && textureId == 3 && ctx_.textureOverride < 0) {
   multiplyVertexColors(ctx_.faceState, red, green, blue);
   (faces_.*kFaceRenderers[face])(block, x, y, z, 38);
  }
  drewAnyFace = true;
 }
 ctx_.faceState.useAo = false;
 return drewAnyFace;
}
bool CubeBlockRenderer::renderFlat(
    net::minecraft::block::Block& block, int x, int y, int z, float red, float green, float blue) {
 if(ctx_.blockView == nullptr) {
  return false;
 }
 return renderUnlitFaces(ctx_, faces_, block, x, y, z, red, green, blue);
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
 const float inset = 0.0625f;
 const auto savedBounds = ctx_.renderBounds;
 ctx_.renderBounds = {inset, 0.0, inset, 1.0f - inset, 1.0, 1.0f - inset};
 const bool drewAnyFace = renderUnlitFaces(ctx_, faces_, block, x, y, z, red, green, blue);
 ctx_.renderBounds = savedBounds;
 return drewAnyFace;
}
} // namespace net::minecraft::client::render::block
