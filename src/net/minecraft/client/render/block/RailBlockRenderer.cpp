#include "net/minecraft/client/render/block/RailBlockRenderer.hpp"

#include "net/minecraft/block/RailBlock.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {

bool RailBlockRenderer::render(net::minecraft::block::RailBlock& rail, int x, int y, int z)
{
    Tessellator& tessellator = render::INSTANCE;
    int meta = ctx_.blockView->getBlockMeta(x, y, z);
    int tex = rail.getTexture(0, meta);
    if (ctx_.textureOverride >= 0) {
        tex = ctx_.textureOverride;
    }
    if (rail.alwaysStraight) {
        meta &= 7;
    }
    float brightness = rail.getLuminance(ctx_.blockView, x, y, z);
    tessellator.color(brightness, brightness, brightness);
    int texU = (tex & 0xF) << 4;
    int texV = tex & 0xF0;
    double d = (float)texU / 256.0f;
    double d2 = ((float)texU + 15.99f) / 256.0f;
    double d3 = (float)texV / 256.0f;
    double d4 = ((float)texV + 15.99f) / 256.0f;
    const float step = 0.0625f;
    float vx0 = x + 1;
    float vx1 = x + 1;
    float vx2 = x + 0;
    float vx3 = x + 0;
    float vz0 = z + 0;
    float vz1 = z + 1;
    float vz2 = z + 1;
    float vz3 = z + 0;
    float vy0 = (float)y + step;
    float vy1 = (float)y + step;
    float vy2 = (float)y + step;
    float vy3 = (float)y + step;
    if (meta == 1 || meta == 2 || meta == 3 || meta == 7) {
        vx0 = vx3 = (float)(x + 1);
        vx1 = vx2 = (float)(x + 0);
        vz0 = vz1 = (float)(z + 1);
        vz2 = vz3 = (float)(z + 0);
    } else if (meta == 8) {
        vx0 = vx1 = (float)(x + 0);
        vx2 = vx3 = (float)(x + 1);
        vz0 = vz3 = (float)(z + 1);
        vz1 = vz2 = (float)(z + 0);
    } else if (meta == 9) {
        vx0 = vx3 = (float)(x + 0);
        vx1 = vx2 = (float)(x + 1);
        vz0 = vz1 = (float)(z + 0);
        vz2 = vz3 = (float)(z + 1);
    }
    if (meta == 2 || meta == 4) {
        vy0 += 1.0f;
        vy3 += 1.0f;
    } else if (meta == 3 || meta == 5) {
        vy1 += 1.0f;
        vy2 += 1.0f;
    }
    tessellator.vertex(vx0, vy0, vz0, d2, d3);
    tessellator.vertex(vx1, vy1, vz1, d2, d4);
    tessellator.vertex(vx2, vy2, vz2, d, d4);
    tessellator.vertex(vx3, vy3, vz3, d, d3);
    tessellator.vertex(vx3, vy3, vz3, d, d3);
    tessellator.vertex(vx2, vy2, vz2, d, d4);
    tessellator.vertex(vx1, vy1, vz1, d2, d4);
    tessellator.vertex(vx0, vy0, vz0, d2, d3);
    return true;
}

} // namespace net::minecraft::client::render::block
