#include "net/minecraft/block/BedBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/util/math/Facings.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::client::render::block {
bool BedBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
 Tessellator& tessellator = *ctx_.tess;
 ctx_.textureOverride = -1;
 ctx_.faceTextureOverride = -1;
 ctx_.flipTextureHorizontally = false;
 const int blockMeta = ctx_.blockView->getBlockMeta(x, y, z);
 const int direction = net::minecraft::block::BedBlock::getDirection(blockMeta);
 const bool isHead = net::minecraft::block::BedBlock::isHeadOfBed(blockMeta);
 constexpr float shadeBottom = 0.5f;
 constexpr float shadeTop = 1.0f;
 constexpr float shadeNorthSouth = 0.8f;
 constexpr float shadeEastWest = 0.6f;
 const float baseBrightness = block.getLuminance(ctx_.blockView, x, y, z);
 tessellator.color(shadeBottom * baseBrightness, shadeBottom * baseBrightness, shadeBottom * baseBrightness);
 const int footTexture = block.getTexture(0, blockMeta);
 const int footTexU = net::minecraft::block::Block::textureAtlasU(footTexture);
 const int footTexV = net::minecraft::block::Block::textureAtlasV(footTexture);
 const double footUvMin = static_cast<double>(footTexU) / 256.0;
 const double footUvMax = (static_cast<double>(footTexU + 16) - 0.01) / 256.0;
 const double footVvMin = static_cast<double>(footTexV) / 256.0;
 const double footVvMax = (static_cast<double>(footTexV + 16) - 0.01) / 256.0;
 const double footX0 = static_cast<double>(x) + ctx_.renderBounds.minX;
 const double footX1 = static_cast<double>(x) + ctx_.renderBounds.maxX;
 const double footY = static_cast<double>(y) + ctx_.renderBounds.minY + 0.1875;
 const double footZ0 = static_cast<double>(z) + ctx_.renderBounds.minZ;
 const double footZ1 = static_cast<double>(z) + ctx_.renderBounds.maxZ;
 tessellator.vertex(footX0, footY, footZ1, footUvMin, footVvMax);
 tessellator.vertex(footX0, footY, footZ0, footUvMin, footVvMin);
 tessellator.vertex(footX1, footY, footZ0, footUvMax, footVvMin);
 tessellator.vertex(footX1, footY, footZ1, footUvMax, footVvMax);
 const float topBrightness = block.getLuminance(ctx_.blockView, x, y + 1, z);
 tessellator.color(shadeTop * topBrightness, shadeTop * topBrightness, shadeTop * topBrightness);
 const int topTexture = block.getTexture(1, blockMeta);
 const int topTexU = net::minecraft::block::Block::textureAtlasU(topTexture);
 const int topTexV = net::minecraft::block::Block::textureAtlasV(topTexture);
 double topUv00 = static_cast<double>(topTexU) / 256.0;
 double topUv10 = (static_cast<double>(topTexU + 16) - 0.01) / 256.0;
 double topUv01 = static_cast<double>(topTexV) / 256.0;
 double topUv11 = (static_cast<double>(topTexV + 16) - 0.01) / 256.0;
 double topUv20 = topUv00;
 double topUv21 = topUv10;
 double topUv30 = topUv01;
 double topUv31 = topUv01;
 double topUv40 = topUv00;
 double topUv41 = topUv10;
 double topUv50 = topUv11;
 double topUv51 = topUv11;
 if(direction == 0) {
  topUv21 = topUv00;
  topUv30 = topUv11;
  topUv40 = topUv10;
  topUv51 = topUv01;
 } else if(direction == 2) {
  topUv20 = topUv10;
  topUv31 = topUv11;
  topUv41 = topUv00;
  topUv50 = topUv01;
 } else if(direction == 3) {
  topUv20 = topUv10;
  topUv31 = topUv11;
  topUv41 = topUv00;
  topUv50 = topUv01;
  topUv21 = topUv00;
  topUv30 = topUv11;
  topUv40 = topUv10;
  topUv51 = topUv01;
 }
 const double topX0 = static_cast<double>(x) + ctx_.renderBounds.minX;
 const double topX1 = static_cast<double>(x) + ctx_.renderBounds.maxX;
 const double topY = static_cast<double>(y) + ctx_.renderBounds.maxY;
 const double topZ0 = static_cast<double>(z) + ctx_.renderBounds.minZ;
 const double topZ1 = static_cast<double>(z) + ctx_.renderBounds.maxZ;
 tessellator.vertex(topX1, topY, topZ1, topUv40, topUv50);
 tessellator.vertex(topX1, topY, topZ0, topUv20, topUv30);
 tessellator.vertex(topX0, topY, topZ0, topUv21, topUv31);
 tessellator.vertex(topX0, topY, topZ1, topUv41, topUv51);
 int skipFaceDir = net::minecraft::util::math::Facings::TO_DIR[direction];
 if(isHead) {
  skipFaceDir =
      net::minecraft::util::math::Facings::TO_DIR[net::minecraft::util::math::Facings::OPPOSITE[direction]];
 }
 int sideFaceTex = 4;
 switch(direction) {
 case 2:
  break;
 case 0:
  sideFaceTex = 5;
  break;
 case 3:
  sideFaceTex = 2;
  break;
 case 1:
  sideFaceTex = 3;
  break;
 default:
  break;
 }
 if(skipFaceDir != 2 && (ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z - 1, 2))) {
  float northBrightness = block.getLuminance(ctx_.blockView, x, y, z - 1);
  if(ctx_.renderBounds.minZ > 0.0) {
   northBrightness = baseBrightness;
  }
  tessellator.color(
      shadeNorthSouth * northBrightness, shadeNorthSouth * northBrightness, shadeNorthSouth * northBrightness);
  ctx_.flipTextureHorizontally = sideFaceTex == 2;
  faces_.renderEastFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 2));
 }
 if(skipFaceDir != 3 && (ctx_.skipFaceCulling || ctx_.isSideVisible(block, x, y, z + 1, 3))) {
  float southBrightness = block.getLuminance(ctx_.blockView, x, y, z + 1);
  if(ctx_.renderBounds.maxZ < 1.0) {
   southBrightness = baseBrightness;
  }
  tessellator.color(
      shadeNorthSouth * southBrightness, shadeNorthSouth * southBrightness, shadeNorthSouth * southBrightness);
  ctx_.flipTextureHorizontally = sideFaceTex == 3;
  faces_.renderWestFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 3));
 }
 if(skipFaceDir != 4 && (ctx_.skipFaceCulling || ctx_.isSideVisible(block, x - 1, y, z, 4))) {
  float westBrightness = block.getLuminance(ctx_.blockView, x - 1, y, z);
  if(ctx_.renderBounds.minX > 0.0) {
   westBrightness = baseBrightness;
  }
  tessellator.color(
      shadeEastWest * westBrightness, shadeEastWest * westBrightness, shadeEastWest * westBrightness);
  ctx_.flipTextureHorizontally = sideFaceTex == 4;
  faces_.renderNorthFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 4));
 }
 if(skipFaceDir != 5 && (ctx_.skipFaceCulling || ctx_.isSideVisible(block, x + 1, y, z, 5))) {
  float eastBrightness = block.getLuminance(ctx_.blockView, x + 1, y, z);
  if(ctx_.renderBounds.maxX < 1.0) {
   eastBrightness = baseBrightness;
  }
  tessellator.color(
      shadeEastWest * eastBrightness, shadeEastWest * eastBrightness, shadeEastWest * eastBrightness);
  ctx_.flipTextureHorizontally = sideFaceTex == 5;
  faces_.renderSouthFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 5));
 }
 ctx_.flipTextureHorizontally = false;
 return true;
}
} // namespace net::minecraft::client::render::block
