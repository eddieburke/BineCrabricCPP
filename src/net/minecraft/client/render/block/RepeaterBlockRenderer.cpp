#include "net/minecraft/client/render/block/RepeaterBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/RepeaterBlock.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {
bool RepeaterBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{

        int n = ctx_.blockView->getBlockMeta(x, y, z);
        int n2 = n & 3;
        int n3 = (n & 0xC) >> 2;
        cube_.renderBlock(block, x, y, z);
        Tessellator& tessellator = render::INSTANCE;
        float f = block.getLuminance(ctx_.blockView, x, y, z);
        if (net::minecraft::block::Block::BLOCKS_LIGHT_LUMINANCE[block.id] > 0) {
            f = (f + 1.0f) * 0.5f;
        }
        tessellator.color(f, f, f);
        double d = -0.1875;
        double d2 = 0.0;
        double d3 = 0.0;
        double d4 = 0.0;
        double d5 = 0.0;
        switch (n2) {
            case 0: {
                d5 = -0.3125;
                d3 = net::minecraft::block::RepeaterBlock::RENDER_OFFSET[n3];
                break;
            }
            case 2: {
                d5 = 0.3125;
                d3 = -net::minecraft::block::RepeaterBlock::RENDER_OFFSET[n3];
                break;
            }
            case 3: {
                d4 = -0.3125;
                d2 = net::minecraft::block::RepeaterBlock::RENDER_OFFSET[n3];
                break;
            }
            case 1: {
                d4 = 0.3125;
                d2 = -net::minecraft::block::RepeaterBlock::RENDER_OFFSET[n3];
            }
        }
        torch_.renderTiltedTorch(block, (double)x + d2, (double)y + d, (double)z + d3, 0.0, 0.0);
        torch_.renderTiltedTorch(block, (double)x + d4, (double)y + d, (double)z + d5, 0.0, 0.0);
        int n4 = block.getTexture(1);
        int n5 = (n4 & 0xF) << 4;
        int n6 = n4 & 0xF0;
        double d6 = (float)n5 / 256.0f;
        double d7 = ((float)n5 + 15.99f) / 256.0f;
        double d8 = (float)n6 / 256.0f;
        double d9 = ((float)n6 + 15.99f) / 256.0f;
        float f2 = 0.125f;
        float f3 = x + 1;
        float f4 = x + 1;
        float f5 = x + 0;
        float f6 = x + 0;
        float f7 = z + 0;
        float f8 = z + 1;
        float f9 = z + 1;
        float f10 = z + 0;
        float f11 = (float)y + f2;
        if (n2 == 2) {
            f3 = f4 = (float)(x + 0);
            f5 = f6 = (float)(x + 1);
            f7 = f10 = (float)(z + 1);
            f8 = f9 = (float)(z + 0);
        } else if (n2 == 3) {
            f3 = f6 = (float)(x + 0);
            f4 = f5 = (float)(x + 1);
            f7 = f8 = (float)(z + 0);
            f9 = f10 = (float)(z + 1);
        } else if (n2 == 1) {
            f3 = f6 = (float)(x + 1);
            f4 = f5 = (float)(x + 0);
            f7 = f8 = (float)(z + 1);
            f9 = f10 = (float)(z + 0);
        }
        tessellator.vertex(f6, f11, f10, d6, d8);
        tessellator.vertex(f5, f11, f9, d6, d9);
        tessellator.vertex(f4, f11, f8, d7, d9);
        tessellator.vertex(f3, f11, f7, d7, d8);
        return true;
    
}

} // namespace net::minecraft::client::render::block

