#include "net/minecraft/client/render/block/LadderBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {

bool LadderBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{
    Tessellator& tessellator = render::INSTANCE;
    int tex = block.getTexture(0);
    if (ctx_.textureOverride >= 0) {
        tex = ctx_.textureOverride;
    }
    float brightness = block.getLuminance(ctx_.blockView, x, y, z);
    tessellator.color(brightness, brightness, brightness);
    int texU = (tex & 0xF) << 4;
    int texV = tex & 0xF0;
    double d = (float)texU / 256.0f;
    double d2 = ((float)texU + 15.99f) / 256.0f;
    double d3 = (float)texV / 256.0f;
    double d4 = ((float)texV + 15.99f) / 256.0f;
    int meta = ctx_.blockView->getBlockMeta(x, y, z);
    const float offset = 0.05f;
    if (meta == 5) {
        tessellator.vertex((float)x + offset, (float)(y + 1), (float)(z + 1), d, d3);
        tessellator.vertex((float)x + offset, (float)(y + 0), (float)(z + 1), d, d4);
        tessellator.vertex((float)x + offset, (float)(y + 0), (float)(z + 0), d2, d4);
        tessellator.vertex((float)x + offset, (float)(y + 1), (float)(z + 0), d2, d3);
    }
    if (meta == 4) {
        tessellator.vertex((float)(x + 1) - offset, (float)(y + 0), (float)(z + 1), d2, d4);
        tessellator.vertex((float)(x + 1) - offset, (float)(y + 1), (float)(z + 1), d2, d3);
        tessellator.vertex((float)(x + 1) - offset, (float)(y + 1), (float)(z + 0), d, d3);
        tessellator.vertex((float)(x + 1) - offset, (float)(y + 0), (float)(z + 0), d, d4);
    }
    if (meta == 3) {
        tessellator.vertex((float)(x + 1), (float)(y + 0), (float)z + offset, d2, d4);
        tessellator.vertex((float)(x + 1), (float)(y + 1), (float)z + offset, d2, d3);
        tessellator.vertex((float)(x + 0), (float)(y + 1), (float)z + offset, d, d3);
        tessellator.vertex((float)(x + 0), (float)(y + 0), (float)z + offset, d, d4);
    }
    if (meta == 2) {
        tessellator.vertex((float)(x + 1), (float)(y + 1), (float)(z + 1) - offset, d, d3);
        tessellator.vertex((float)(x + 1), (float)(y + 0), (float)(z + 1) - offset, d, d4);
        tessellator.vertex((float)(x + 0), (float)(y + 0), (float)(z + 1) - offset, d2, d4);
        tessellator.vertex((float)(x + 0), (float)(y + 1), (float)(z + 1) - offset, d2, d3);
    }
    return true;
}

} // namespace net::minecraft::client::render::block
