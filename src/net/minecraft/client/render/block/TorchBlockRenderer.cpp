#include "net/minecraft/client/render/block/TorchBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {

bool TorchBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{
    int n = ctx_.blockView->getBlockMeta(x, y, z);
    Tessellator& tessellator = render::INSTANCE;
    float f = block.getLuminance(ctx_.blockView, x, y, z);
    if (net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
        f = 1.0f;
    }
    tessellator.color(f, f, f);
    double d = 0.4f;
    double d2 = 0.5 - d;
    double d3 = 0.2f;
    if (n == 1) {
        renderTiltedTorch(block, (double)x - d2, (double)y + d3, z, -d, 0.0);
    } else if (n == 2) {
        renderTiltedTorch(block, (double)x + d2, (double)y + d3, z, d, 0.0);
    } else if (n == 3) {
        renderTiltedTorch(block, x, (double)y + d3, (double)z - d2, 0.0, -d);
    } else if (n == 4) {
        renderTiltedTorch(block, x, (double)y + d3, (double)z + d2, 0.0, d);
    } else {
        renderTiltedTorch(block, x, y, z, 0.0, 0.0);
    }
    return true;
}

void TorchBlockRenderer::renderTiltedTorch(net::minecraft::block::Block& block, double x, double y, double z, double xTilt, double zTilt)
{
    Tessellator& tessellator = render::INSTANCE;
    int n = block.getTexture(0);
    if (ctx_.textureOverride >= 0) {
        n = ctx_.textureOverride;
    }
    int n2 = (n & 0xF) << 4;
    int n3 = n & 0xF0;
    float f = (float)n2 / 256.0f;
    float f2 = ((float)n2 + 15.99f) / 256.0f;
    float f3 = (float)n3 / 256.0f;
    float f4 = ((float)n3 + 15.99f) / 256.0f;
    double d = (double)f + 0.02734375;
    double d2 = (double)f3 + 0.0234375;
    double d3 = (double)f + 0.03515625;
    double d4 = (double)f3 + 0.03125;
    double d5 = (x += 0.5) - 0.5;
    double d6 = x + 0.5;
    double d7 = (z += 0.5) - 0.5;
    double d8 = z + 0.5;
    double d9 = 0.0625;
    double d10 = 0.625;
    tessellator.vertex(x + xTilt * (1.0 - d10) - d9, y + d10, z + zTilt * (1.0 - d10) - d9, d, d2);
    tessellator.vertex(x + xTilt * (1.0 - d10) - d9, y + d10, z + zTilt * (1.0 - d10) + d9, d, d4);
    tessellator.vertex(x + xTilt * (1.0 - d10) + d9, y + d10, z + zTilt * (1.0 - d10) + d9, d3, d4);
    tessellator.vertex(x + xTilt * (1.0 - d10) + d9, y + d10, z + zTilt * (1.0 - d10) - d9, d3, d2);
    tessellator.vertex(x - d9, y + 1.0, d7, f, f3);
    tessellator.vertex(x - d9 + xTilt, y + 0.0, d7 + zTilt, f, f4);
    tessellator.vertex(x - d9 + xTilt, y + 0.0, d8 + zTilt, f2, f4);
    tessellator.vertex(x - d9, y + 1.0, d8, f2, f3);
    tessellator.vertex(x + d9, y + 1.0, d8, f, f3);
    tessellator.vertex(x + xTilt + d9, y + 0.0, d8 + zTilt, f, f4);
    tessellator.vertex(x + xTilt + d9, y + 0.0, d7 + zTilt, f2, f4);
    tessellator.vertex(x + d9, y + 1.0, d7, f2, f3);
    tessellator.vertex(d5, y + 1.0, z + d9, f, f3);
    tessellator.vertex(d5 + xTilt, y + 0.0, z + d9 + zTilt, f, f4);
    tessellator.vertex(d6 + xTilt, y + 0.0, z + d9 + zTilt, f2, f4);
    tessellator.vertex(d6, y + 1.0, z + d9, f2, f3);
    tessellator.vertex(d6, y + 1.0, z - d9, f, f3);
    tessellator.vertex(d6 + xTilt, y + 0.0, z - d9 + zTilt, f, f4);
    tessellator.vertex(d5 + xTilt, y + 0.0, z - d9 + zTilt, f2, f4);
    tessellator.vertex(d5, y + 1.0, z - d9, f2, f3);
}

} // namespace net::minecraft::client::render::block
