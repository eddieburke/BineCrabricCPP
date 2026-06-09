#include "net/minecraft/client/render/block/FireBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/FireBlock.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {
bool FireBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{

        Tessellator& tessellator = render::INSTANCE;
        int n = block.getTexture(0);
        if (ctx_.textureOverride >= 0) {
            n = ctx_.textureOverride;
        }
        float f = block.getLuminance(ctx_.blockView, x, y, z);
        tessellator.color(f, f, f);
        int n2 = (n & 0xF) << 4;
        int n3 = n & 0xF0;
        double d = (float)n2 / 256.0f;
        double d2 = ((float)n2 + 15.99f) / 256.0f;
        double d3 = (float)n3 / 256.0f;
        double d4 = ((float)n3 + 15.99f) / 256.0f;
        float f2 = 1.4f;
        if (ctx_.blockView->shouldSuffocate(x, y - 1, z) || net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x, y - 1, z)) {
            double d5 = (double)x + 0.5 + 0.2;
            double d6 = (double)x + 0.5 - 0.2;
            double d7 = (double)z + 0.5 + 0.2;
            double d8 = (double)z + 0.5 - 0.2;
            double d9 = (double)x + 0.5 - 0.3;
            double d10 = (double)x + 0.5 + 0.3;
            double d11 = (double)z + 0.5 - 0.3;
            double d12 = (double)z + 0.5 + 0.3;
            tessellator.vertex(d9, (float)y + f2, z + 1, d2, d3);
            tessellator.vertex(d5, y + 0, z + 1, d2, d4);
            tessellator.vertex(d5, y + 0, z + 0, d, d4);
            tessellator.vertex(d9, (float)y + f2, z + 0, d, d3);
            tessellator.vertex(d10, (float)y + f2, z + 0, d2, d3);
            tessellator.vertex(d6, y + 0, z + 0, d2, d4);
            tessellator.vertex(d6, y + 0, z + 1, d, d4);
            tessellator.vertex(d10, (float)y + f2, z + 1, d, d3);
            d = (float)n2 / 256.0f;
            d2 = ((float)n2 + 15.99f) / 256.0f;
            d3 = (float)(n3 + 16) / 256.0f;
            d4 = ((float)n3 + 15.99f + 16.0f) / 256.0f;
            tessellator.vertex(x + 1, (float)y + f2, d12, d2, d3);
            tessellator.vertex(x + 1, y + 0, d8, d2, d4);
            tessellator.vertex(x + 0, y + 0, d8, d, d4);
            tessellator.vertex(x + 0, (float)y + f2, d12, d, d3);
            tessellator.vertex(x + 0, (float)y + f2, d11, d2, d3);
            tessellator.vertex(x + 0, y + 0, d7, d2, d4);
            tessellator.vertex(x + 1, y + 0, d7, d, d4);
            tessellator.vertex(x + 1, (float)y + f2, d11, d, d3);
            d5 = (double)x + 0.5 - 0.5;
            d6 = (double)x + 0.5 + 0.5;
            d7 = (double)z + 0.5 - 0.5;
            d8 = (double)z + 0.5 + 0.5;
            d9 = (double)x + 0.5 - 0.4;
            d10 = (double)x + 0.5 + 0.4;
            d11 = (double)z + 0.5 - 0.4;
            d12 = (double)z + 0.5 + 0.4;
            tessellator.vertex(d9, (float)y + f2, z + 0, d, d3);
            tessellator.vertex(d5, y + 0, z + 0, d, d4);
            tessellator.vertex(d5, y + 0, z + 1, d2, d4);
            tessellator.vertex(d9, (float)y + f2, z + 1, d2, d3);
            tessellator.vertex(d10, (float)y + f2, z + 1, d, d3);
            tessellator.vertex(d6, y + 0, z + 1, d, d4);
            tessellator.vertex(d6, y + 0, z + 0, d2, d4);
            tessellator.vertex(d10, (float)y + f2, z + 0, d2, d3);
            d = (float)n2 / 256.0f;
            d2 = ((float)n2 + 15.99f) / 256.0f;
            d3 = (float)n3 / 256.0f;
            d4 = ((float)n3 + 15.99f) / 256.0f;
            tessellator.vertex(x + 0, (float)y + f2, d12, d, d3);
            tessellator.vertex(x + 0, y + 0, d8, d, d4);
            tessellator.vertex(x + 1, y + 0, d8, d2, d4);
            tessellator.vertex(x + 1, (float)y + f2, d12, d2, d3);
            tessellator.vertex(x + 1, (float)y + f2, d11, d, d3);
            tessellator.vertex(x + 1, y + 0, d7, d, d4);
            tessellator.vertex(x + 0, y + 0, d7, d2, d4);
            tessellator.vertex(x + 0, (float)y + f2, d11, d2, d3);
        } else {
            double d13;
            float f3 = 0.2f;
            float f4 = 0.0625f;
            if (((x + y + z) & 1) == 1) {
                d = (float)n2 / 256.0f;
                d2 = ((float)n2 + 15.99f) / 256.0f;
                d3 = (float)(n3 + 16) / 256.0f;
                d4 = ((float)n3 + 15.99f + 16.0f) / 256.0f;
            }
            if ((((x / 2) + (y / 2) + (z / 2)) & 1) == 1) {
                d13 = d2;
                d2 = d;
                d = d13;
            }
            if (net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x - 1, y, z)) {
                tessellator.vertex((float)x + f3, (float)y + f2 + f4, z + 1, d2, d3);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 1, d2, d4);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex((float)x + f3, (float)y + f2 + f4, z + 0, d, d3);
                tessellator.vertex((float)x + f3, (float)y + f2 + f4, z + 0, d, d3);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 1, d2, d4);
                tessellator.vertex((float)x + f3, (float)y + f2 + f4, z + 1, d2, d3);
            }
            if (net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x + 1, y, z)) {
                tessellator.vertex((float)(x + 1) - f3, (float)y + f2 + f4, z + 0, d, d3);
                tessellator.vertex(x + 1 - 0, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex(x + 1 - 0, (float)(y + 0) + f4, z + 1, d2, d4);
                tessellator.vertex((float)(x + 1) - f3, (float)y + f2 + f4, z + 1, d2, d3);
                tessellator.vertex((float)(x + 1) - f3, (float)y + f2 + f4, z + 1, d2, d3);
                tessellator.vertex(x + 1 - 0, (float)(y + 0) + f4, z + 1, d2, d4);
                tessellator.vertex(x + 1 - 0, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex((float)(x + 1) - f3, (float)y + f2 + f4, z + 0, d, d3);
            }
            if (net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x, y, z - 1)) {
                tessellator.vertex(x + 0, (float)y + f2 + f4, (float)z + f3, d2, d3);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 0, d2, d4);
                tessellator.vertex(x + 1, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex(x + 1, (float)y + f2 + f4, (float)z + f3, d, d3);
                tessellator.vertex(x + 1, (float)y + f2 + f4, (float)z + f3, d, d3);
                tessellator.vertex(x + 1, (float)(y + 0) + f4, z + 0, d, d4);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 0, d2, d4);
                tessellator.vertex(x + 0, (float)y + f2 + f4, (float)z + f3, d2, d3);
            }
            if (net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x, y, z + 1)) {
                tessellator.vertex(x + 1, (float)y + f2 + f4, (float)(z + 1) - f3, d, d3);
                tessellator.vertex(x + 1, (float)(y + 0) + f4, z + 1 - 0, d, d4);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 1 - 0, d2, d4);
                tessellator.vertex(x + 0, (float)y + f2 + f4, (float)(z + 1) - f3, d2, d3);
                tessellator.vertex(x + 0, (float)y + f2 + f4, (float)(z + 1) - f3, d2, d3);
                tessellator.vertex(x + 0, (float)(y + 0) + f4, z + 1 - 0, d2, d4);
                tessellator.vertex(x + 1, (float)(y + 0) + f4, z + 1 - 0, d, d4);
                tessellator.vertex(x + 1, (float)y + f2 + f4, (float)(z + 1) - f3, d, d3);
            }
            if (net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x, y + 1, z)) {
                d13 = (double)x + 0.5 + 0.5;
                double d14 = (double)x + 0.5 - 0.5;
                double d15 = (double)z + 0.5 + 0.5;
                double d16 = (double)z + 0.5 - 0.5;
                double d17 = (double)x + 0.5 - 0.5;
                double d18 = (double)x + 0.5 + 0.5;
                double d19 = (double)z + 0.5 - 0.5;
                double d20 = (double)z + 0.5 + 0.5;
                d = (float)n2 / 256.0f;
                d2 = ((float)n2 + 15.99f) / 256.0f;
                d3 = (float)n3 / 256.0f;
                d4 = ((float)n3 + 15.99f) / 256.0f;
                f2 = -0.2f;
                if (((x + ++y + z) & 1) == 0) {
                    tessellator.vertex(d17, (float)y + f2, z + 0, d2, d3);
                    tessellator.vertex(d13, y + 0, z + 0, d2, d4);
                    tessellator.vertex(d13, y + 0, z + 1, d, d4);
                    tessellator.vertex(d17, (float)y + f2, z + 1, d, d3);
                    d = (float)n2 / 256.0f;
                    d2 = ((float)n2 + 15.99f) / 256.0f;
                    d3 = (float)(n3 + 16) / 256.0f;
                    d4 = ((float)n3 + 15.99f + 16.0f) / 256.0f;
                    tessellator.vertex(d18, (float)y + f2, z + 1, d2, d3);
                    tessellator.vertex(d14, y + 0, z + 1, d2, d4);
                    tessellator.vertex(d14, y + 0, z + 0, d, d4);
                    tessellator.vertex(d18, (float)y + f2, z + 0, d, d3);
                } else {
                    tessellator.vertex(x + 0, (float)y + f2, d20, d2, d3);
                    tessellator.vertex(x + 0, y + 0, d16, d2, d4);
                    tessellator.vertex(x + 1, y + 0, d16, d, d4);
                    tessellator.vertex(x + 1, (float)y + f2, d20, d, d3);
                    d = (float)n2 / 256.0f;
                    d2 = ((float)n2 + 15.99f) / 256.0f;
                    d3 = (float)(n3 + 16) / 256.0f;
                    d4 = ((float)n3 + 15.99f + 16.0f) / 256.0f;
                    tessellator.vertex(x + 1, (float)y + f2, d19, d2, d3);
                    tessellator.vertex(x + 1, y + 0, d15, d2, d4);
                    tessellator.vertex(x + 0, y + 0, d15, d, d4);
                    tessellator.vertex(x + 0, (float)y + f2, d19, d, d3);
                }
            }
        }
        return true;
    
}


} // namespace net::minecraft::client::render::block

