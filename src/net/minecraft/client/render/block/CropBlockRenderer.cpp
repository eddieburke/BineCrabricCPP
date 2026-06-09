#include "net/minecraft/client/render/block/CropBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {

bool CropBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{
    Tessellator& tessellator = render::INSTANCE;
    float brightness = block.getLuminance(ctx_.blockView, x, y, z);
    tessellator.color(brightness, brightness, brightness);
    render(block, ctx_.blockView->getBlockMeta(x, y, z), x, (float)y - 0.0625f, z);
    return true;
}

void CropBlockRenderer::render(net::minecraft::block::Block& block, int metadata, double x, double y, double z)
{
    Tessellator& tessellator = render::INSTANCE;
    int tex = block.getTexture(0, metadata);
    if (ctx_.textureOverride >= 0) {
        tex = ctx_.textureOverride;
    }
    int texU = (tex & 0xF) << 4;
    int texV = tex & 0xF0;
    double uMin = (float)texU / 256.0f;
    double uMax = ((float)texU + 15.99f) / 256.0f;
    double vMin = (float)texV / 256.0f;
    double vMax = ((float)texV + 15.99f) / 256.0f;
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

} // namespace net::minecraft::client::render::block
