#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/RepeaterBlock.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::client::render::block {
bool RepeaterBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
 const int blockMeta = ctx_.blockView->getBlockMeta(x, y, z);
 const int direction = blockMeta & 3;
 const int delayIndex = (blockMeta & 0xC) >> 2;
 cube_.renderBlock(block, x, y, z);
 Tessellator& tessellator = *ctx_.tess;
 float brightness = block.getLuminance(ctx_.blockView, x, y, z);
 if(block.emission() > 0) {
  brightness = (brightness + 1.0f) * 0.5f;
 }
 tessellator.color(brightness, brightness, brightness);
 const double torchYOffset = -0.1875;
 double torchXOffsetA = 0.0;
 double torchZOffsetA = 0.0;
 double torchXOffsetB = 0.0;
 double torchZOffsetB = 0.0;
 switch(direction) {
 case 0:
  torchZOffsetB = -0.3125;
  torchZOffsetA = net::minecraft::block::RepeaterBlock::RENDER_OFFSET[delayIndex];
  break;
 case 2:
  torchZOffsetB = 0.3125;
  torchZOffsetA = -net::minecraft::block::RepeaterBlock::RENDER_OFFSET[delayIndex];
  break;
 case 3:
  torchXOffsetB = -0.3125;
  torchXOffsetA = net::minecraft::block::RepeaterBlock::RENDER_OFFSET[delayIndex];
  break;
 case 1:
  torchXOffsetB = 0.3125;
  torchXOffsetA = -net::minecraft::block::RepeaterBlock::RENDER_OFFSET[delayIndex];
  break;
 default:
  break;
 }
 torch_.renderTiltedTorch(block,
                          static_cast<double>(x) + torchXOffsetA,
                          static_cast<double>(y) + torchYOffset,
                          static_cast<double>(z) + torchZOffsetA,
                          0.0,
                          0.0);
 torch_.renderTiltedTorch(block,
                          static_cast<double>(x) + torchXOffsetB,
                          static_cast<double>(y) + torchYOffset,
                          static_cast<double>(z) + torchZOffsetB,
                          0.0,
                          0.0);
 const net::minecraft::block::TerrainAtlasUv slabUv =
     net::minecraft::block::Block::terrainTileUv(ctx_.resolveTexture(1, block.getTexture(1)));
 const double uvMinU = slabUv.uMin;
 const double uvMaxU = slabUv.uMax;
 const double uvMinV = slabUv.vMin;
 const double uvMaxV = slabUv.vMax;
 constexpr float slabHeight = 0.125f;
 float vx1 = static_cast<float>(x + 1);
 float vx2 = static_cast<float>(x + 1);
 float vx3 = static_cast<float>(x + 0);
 float vx4 = static_cast<float>(x + 0);
 float vz1 = static_cast<float>(z + 0);
 float vz2 = static_cast<float>(z + 1);
 float vz3 = static_cast<float>(z + 1);
 float vz4 = static_cast<float>(z + 0);
 const float vy = static_cast<float>(y) + slabHeight;
 if(direction == 2) {
  vx1 = vx2 = static_cast<float>(x + 0);
  vx3 = vx4 = static_cast<float>(x + 1);
  vz1 = vz4 = static_cast<float>(z + 1);
  vz2 = vz3 = static_cast<float>(z + 0);
 } else if(direction == 3) {
  vx1 = vx4 = static_cast<float>(x + 0);
  vx2 = vx3 = static_cast<float>(x + 1);
  vz1 = vz2 = static_cast<float>(z + 0);
  vz3 = vz4 = static_cast<float>(z + 1);
 } else if(direction == 1) {
  vx1 = vx4 = static_cast<float>(x + 1);
  vx2 = vx3 = static_cast<float>(x + 0);
  vz1 = vz2 = static_cast<float>(z + 1);
  vz3 = vz4 = static_cast<float>(z + 0);
 }
 tessellator.vertex(vx4, vy, vz4, uvMinU, uvMinV);
 tessellator.vertex(vx3, vy, vz3, uvMinU, uvMaxV);
 tessellator.vertex(vx2, vy, vz2, uvMaxU, uvMaxV);
 tessellator.vertex(vx1, vy, vz1, uvMaxU, uvMinV);
 return true;
}
} // namespace net::minecraft::client::render::block
