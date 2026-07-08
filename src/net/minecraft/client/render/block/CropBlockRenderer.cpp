#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {
bool CropBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
    Tessellator& tessellator = *ctx_.tess;
    float brightness = block.getLuminance(ctx_.blockView, x, y, z);
    tessellator.color(brightness, brightness, brightness);
    render(block, ctx_.blockView->getBlockMeta(x, y, z), x, (float) y - 0.0625f, z);
    return true;
}

void CropBlockRenderer::render(net::minecraft::block::Block& block, int metadata, double x, double y, double z) {
    Tessellator& tessellator = *ctx_.tess;
    const int tex = ctx_.resolveTexture(0, block.getTexture(0, metadata));
    const net::minecraft::block::TerrainAtlasUv uv = net::minecraft::block::Block::terrainTileUv(tex);
    const double uMin = uv.uMin;
    const double uMax = uv.uMax;
    const double vMin = uv.vMin;
    const double vMax = uv.vMax;
    double x0 = x + 0.5 - 0.25;
    double x1 = x + 0.5 + 0.25;
    double z0 = z + 0.5 - 0.5;
    double z1 = z + 0.5 + 0.5;
    tessellator.vertex(x0, y + 1.0, z0, uMin, vMin);
    tessellator.vertex(x0, y + 0.0, z0, uMin, vMax);
    tessellator.vertex(x0, y + 0.0, z1, uMax, vMax);
    tessellator.vertex(x0, y + 1.0, z1, uMax, vMin);
    tessellator.vertex(x0, y + 1.0, z1, uMin, vMin);
    tessellator.vertex(x0, y + 0.0, z1, uMin, vMax);
    tessellator.vertex(x0, y + 0.0, z0, uMax, vMax);
    tessellator.vertex(x0, y + 1.0, z0, uMax, vMin);
    tessellator.vertex(x1, y + 1.0, z1, uMin, vMin);
    tessellator.vertex(x1, y + 0.0, z1, uMin, vMax);
    tessellator.vertex(x1, y + 0.0, z0, uMax, vMax);
    tessellator.vertex(x1, y + 1.0, z0, uMax, vMin);
    tessellator.vertex(x1, y + 1.0, z0, uMin, vMin);
    tessellator.vertex(x1, y + 0.0, z0, uMin, vMax);
    tessellator.vertex(x1, y + 0.0, z1, uMax, vMax);
    tessellator.vertex(x1, y + 1.0, z1, uMax, vMin);
    x0 = x + 0.5 - 0.5;
    x1 = x + 0.5 + 0.5;
    z0 = z + 0.5 - 0.25;
    z1 = z + 0.5 + 0.25;
    tessellator.vertex(x0, y + 1.0, z0, uMin, vMin);
    tessellator.vertex(x0, y + 0.0, z0, uMin, vMax);
    tessellator.vertex(x1, y + 0.0, z0, uMax, vMax);
    tessellator.vertex(x1, y + 1.0, z0, uMax, vMin);
    tessellator.vertex(x1, y + 1.0, z0, uMin, vMin);
    tessellator.vertex(x1, y + 0.0, z0, uMin, vMax);
    tessellator.vertex(x0, y + 0.0, z0, uMax, vMax);
    tessellator.vertex(x0, y + 1.0, z0, uMax, vMin);
    tessellator.vertex(x1, y + 1.0, z1, uMin, vMin);
    tessellator.vertex(x1, y + 0.0, z1, uMin, vMax);
    tessellator.vertex(x0, y + 0.0, z1, uMax, vMax);
    tessellator.vertex(x0, y + 1.0, z1, uMax, vMin);
    tessellator.vertex(x0, y + 1.0, z1, uMin, vMin);
    tessellator.vertex(x0, y + 0.0, z1, uMin, vMax);
    tessellator.vertex(x1, y + 0.0, z1, uMax, vMax);
    tessellator.vertex(x1, y + 1.0, z1, uMax, vMin);
}
}  // namespace net::minecraft::client::render::block
