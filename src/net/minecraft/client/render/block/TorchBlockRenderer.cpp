#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::client::render::block {
bool TorchBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
  const int blockMeta = ctx_.blockView->getBlockMeta(x, y, z);
  Tessellator& tessellator = *ctx_.tess;
  float brightness = block.getLuminance(ctx_.blockView, x, y, z);
  if(net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
    brightness = 1.0f;
  }
  tessellator.color(brightness, brightness, brightness);
  constexpr double tiltAmount = 0.4;
  constexpr double wallInset = 0.1;
  constexpr double yOffset = 0.2;
  if(blockMeta == 1) {
    renderTiltedTorch(block, static_cast<double>(x) - wallInset, static_cast<double>(y) + yOffset, z, -tiltAmount,
                      0.0);
  } else if(blockMeta == 2) {
    renderTiltedTorch(block, static_cast<double>(x) + wallInset, static_cast<double>(y) + yOffset, z, tiltAmount,
                      0.0);
  } else if(blockMeta == 3) {
    renderTiltedTorch(block, x, static_cast<double>(y) + yOffset, static_cast<double>(z) - wallInset, 0.0,
                      -tiltAmount);
  } else if(blockMeta == 4) {
    renderTiltedTorch(block, x, static_cast<double>(y) + yOffset, static_cast<double>(z) + wallInset, 0.0,
                      tiltAmount);
  } else {
    renderTiltedTorch(block, x, y, z, 0.0, 0.0);
  }
  return true;
}
void TorchBlockRenderer::renderTiltedTorch(net::minecraft::block::Block& block, double x, double y, double z,
                                           double xTilt, double zTilt) {
  Tessellator& tessellator = *ctx_.tess;
  const net::minecraft::block::TerrainAtlasUv atlasUv =
      net::minecraft::block::Block::terrainTileUv(ctx_.resolveTexture(0, block.getTexture(0)));
  const float atlasUMin = static_cast<float>(atlasUv.uMin);
  const float atlasUMax = static_cast<float>(atlasUv.uMax);
  const float atlasVMin = static_cast<float>(atlasUv.vMin);
  const float atlasVMax = static_cast<float>(atlasUv.vMax);
  const double capUMin = static_cast<double>(atlasUMin) + 0.02734375;
  const double capUMax = static_cast<double>(atlasVMin) + 0.0234375;
  const double capVMin = static_cast<double>(atlasUMin) + 0.03515625;
  const double capVMax = static_cast<double>(atlasVMin) + 0.03125;
  const double x0 = (x += 0.5) - 0.5;
  const double x1 = x + 0.5;
  const double z0 = (z += 0.5) - 0.5;
  const double z1 = z + 0.5;
  constexpr double stickHalf = 0.0625;
  constexpr double capHeight = 0.625;
  tessellator.vertex(x + xTilt * (1.0 - capHeight) - stickHalf, y + capHeight,
                     z + zTilt * (1.0 - capHeight) - stickHalf, capUMin, capUMax);
  tessellator.vertex(x + xTilt * (1.0 - capHeight) - stickHalf, y + capHeight,
                     z + zTilt * (1.0 - capHeight) + stickHalf, capUMin, capVMax);
  tessellator.vertex(x + xTilt * (1.0 - capHeight) + stickHalf, y + capHeight,
                     z + zTilt * (1.0 - capHeight) + stickHalf, capVMin, capVMax);
  tessellator.vertex(x + xTilt * (1.0 - capHeight) + stickHalf, y + capHeight,
                     z + zTilt * (1.0 - capHeight) - stickHalf, capVMin, capUMax);
  tessellator.vertex(x - stickHalf, y + 1.0, z0, atlasUMin, atlasVMin);
  tessellator.vertex(x - stickHalf + xTilt, y + 0.0, z0 + zTilt, atlasUMin, atlasVMax);
  tessellator.vertex(x - stickHalf + xTilt, y + 0.0, z1 + zTilt, atlasUMax, atlasVMax);
  tessellator.vertex(x - stickHalf, y + 1.0, z1, atlasUMax, atlasVMin);
  tessellator.vertex(x + stickHalf, y + 1.0, z1, atlasUMin, atlasVMin);
  tessellator.vertex(x + xTilt + stickHalf, y + 0.0, z1 + zTilt, atlasUMin, atlasVMax);
  tessellator.vertex(x + xTilt + stickHalf, y + 0.0, z0 + zTilt, atlasUMax, atlasVMax);
  tessellator.vertex(x + stickHalf, y + 1.0, z0, atlasUMax, atlasVMin);
  tessellator.vertex(x0, y + 1.0, z + stickHalf, atlasUMin, atlasVMin);
  tessellator.vertex(x0 + xTilt, y + 0.0, z + stickHalf + zTilt, atlasUMin, atlasVMax);
  tessellator.vertex(x1 + xTilt, y + 0.0, z + stickHalf + zTilt, atlasUMax, atlasVMax);
  tessellator.vertex(x1, y + 1.0, z + stickHalf, atlasUMax, atlasVMin);
  tessellator.vertex(x1, y + 1.0, z - stickHalf, atlasUMin, atlasVMin);
  tessellator.vertex(x1 + xTilt, y + 0.0, z - stickHalf + zTilt, atlasUMin, atlasVMax);
  tessellator.vertex(x0 + xTilt, y + 0.0, z - stickHalf + zTilt, atlasUMax, atlasVMax);
  tessellator.vertex(x0, y + 1.0, z - stickHalf, atlasUMax, atlasVMin);
}
} // namespace net::minecraft::client::render::block
