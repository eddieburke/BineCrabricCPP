#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::client::render::block {
bool LadderBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
  Tessellator& tessellator = *ctx_.tess;
  const int tex = ctx_.resolveTexture(0, block.getTexture(0));
  const net::minecraft::block::TerrainAtlasUv uv = net::minecraft::block::Block::terrainTileUv(tex);
  float brightness = block.getLuminance(ctx_.blockView, x, y, z);
  tessellator.color(brightness, brightness, brightness);
  const double uMin = uv.uMin;
  const double uMax = uv.uMax;
  const double vMin = uv.vMin;
  const double vMax = uv.vMax;
  int meta = ctx_.blockView->getBlockMeta(x, y, z);
  const float offset = 0.05f;
  if(meta == 5) {
    tessellator.vertex((float)x + offset, (float)(y + 1), (float)(z + 1), uMin, vMin);
    tessellator.vertex((float)x + offset, (float)(y + 0), (float)(z + 1), uMin, vMax);
    tessellator.vertex((float)x + offset, (float)(y + 0), (float)(z + 0), uMax, vMax);
    tessellator.vertex((float)x + offset, (float)(y + 1), (float)(z + 0), uMax, vMin);
  }
  if(meta == 4) {
    tessellator.vertex((float)(x + 1) - offset, (float)(y + 0), (float)(z + 1), uMax, vMax);
    tessellator.vertex((float)(x + 1) - offset, (float)(y + 1), (float)(z + 1), uMax, vMin);
    tessellator.vertex((float)(x + 1) - offset, (float)(y + 1), (float)(z + 0), uMin, vMin);
    tessellator.vertex((float)(x + 1) - offset, (float)(y + 0), (float)(z + 0), uMin, vMax);
  }
  if(meta == 3) {
    tessellator.vertex((float)(x + 1), (float)(y + 0), (float)z + offset, uMax, vMax);
    tessellator.vertex((float)(x + 1), (float)(y + 1), (float)z + offset, uMax, vMin);
    tessellator.vertex((float)(x + 0), (float)(y + 1), (float)z + offset, uMin, vMin);
    tessellator.vertex((float)(x + 0), (float)(y + 0), (float)z + offset, uMin, vMax);
  }
  if(meta == 2) {
    tessellator.vertex((float)(x + 1), (float)(y + 1), (float)(z + 1) - offset, uMin, vMin);
    tessellator.vertex((float)(x + 1), (float)(y + 0), (float)(z + 1) - offset, uMin, vMax);
    tessellator.vertex((float)(x + 0), (float)(y + 0), (float)(z + 1) - offset, uMax, vMax);
    tessellator.vertex((float)(x + 0), (float)(y + 1), (float)(z + 1) - offset, uMax, vMin);
  }
  return true;
}
} // namespace net::minecraft::client::render::block
