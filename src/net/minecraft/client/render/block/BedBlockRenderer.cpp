#include "net/minecraft/client/render/block/BedBlockRenderer.hpp"

#include "net/minecraft/block/BedBlock.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/util/math/Facings.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {

bool BedBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{
    Tessellator& tessellator = render::INSTANCE;
    int n = ctx_.blockView->getBlockMeta(x, y, z);
    int n2 = net::minecraft::block::BedBlock::getDirection(n);
    bool bl = net::minecraft::block::BedBlock::isHeadOfBed(n);
    float f = 0.5f;
    float f2 = 1.0f;
    float f3 = 0.8f;
    float f4 = 0.6f;
    float f5 = f2;
    float f6 = f2;
    float f7 = f2;
    float f8 = f;
    float f9 = f3;
    float f10 = f4;
    float f11 = f;
    float f12 = f3;
    float f13 = f4;
    float f14 = f;
    float f15 = f3;
    float f16 = f4;
    float f17 = block.getLuminance(ctx_.blockView, x, y, z);
    tessellator.color(f8 * f17, f11 * f17, f14 * f17);
    int n3 = block.getTextureId(ctx_.blockView, x, y, z, 0);
    int n4 = (n3 & 0xF) << 4;
    int n5 = n3 & 0xF0;
    double d = (float)n4 / 256.0f;
    double d2 = ((double)(n4 + 16) - 0.01) / 256.0;
    double d3 = (float)n5 / 256.0f;
    double d4 = ((double)(n5 + 16) - 0.01) / 256.0;
    double d5 = (double)x + block.minX;
    double d6 = (double)x + block.maxX;
    double d7 = (double)y + block.minY + 0.1875;
    double d8 = (double)z + block.minZ;
    double d9 = (double)z + block.maxZ;
    tessellator.vertex(d5, d7, d9, d, d4);
    tessellator.vertex(d5, d7, d8, d, d3);
    tessellator.vertex(d6, d7, d8, d2, d3);
    tessellator.vertex(d6, d7, d9, d2, d4);
    float f18 = block.getLuminance(ctx_.blockView, x, y + 1, z);
    tessellator.color(f5 * f18, f6 * f18, f7 * f18);
    n4 = block.getTextureId(ctx_.blockView, x, y, z, 1);
    n5 = (n4 & 0xF) << 4;
    int n6 = n4 & 0xF0;
    double d10 = (float)n5 / 256.0f;
    double d11 = ((double)(n5 + 16) - 0.01) / 256.0;
    double d12 = (float)n6 / 256.0f;
    double d13 = ((double)(n6 + 16) - 0.01) / 256.0;
    double d14 = d10;
    double d15 = d11;
    double d16 = d12;
    double d17 = d12;
    double d18 = d10;
    double d19 = d11;
    double d20 = d13;
    double d21 = d13;
    if (n2 == 0) {
        d15 = d10;
        d16 = d13;
        d18 = d11;
        d21 = d12;
    } else if (n2 == 2) {
        d14 = d11;
        d17 = d13;
        d19 = d10;
        d20 = d12;
    } else if (n2 == 3) {
        d14 = d11;
        d17 = d13;
        d19 = d10;
        d20 = d12;
        d15 = d10;
        d16 = d13;
        d18 = d11;
        d21 = d12;
    }
    double d22 = (double)x + block.minX;
    double d23 = (double)x + block.maxX;
    double d24 = (double)y + block.maxY;
    double d25 = (double)z + block.minZ;
    double d26 = (double)z + block.maxZ;
    tessellator.vertex(d23, d24, d26, d18, d20);
    tessellator.vertex(d23, d24, d25, d14, d16);
    tessellator.vertex(d22, d24, d25, d15, d17);
    tessellator.vertex(d22, d24, d26, d19, d21);
    int n7 = net::minecraft::util::math::Facings::TO_DIR[n2];
    if (bl) {
        n7 = net::minecraft::util::math::Facings::TO_DIR[net::minecraft::util::math::Facings::OPPOSITE[n2]];
    }
    n4 = 4;
    switch (n2) {
    case 2: {
        break;
    }
    case 0: {
        n4 = 5;
        break;
    }
    case 3: {
        n4 = 2;
        break;
    }
    case 1: {
        n4 = 3;
    }
    }
    if (n7 != 2 && (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y, z - 1, 2))) {
        float f19 = block.getLuminance(ctx_.blockView, x, y, z - 1);
        if (block.minZ > 0.0) {
            f19 = f17;
        }
        tessellator.color(f9 * f19, f12 * f19, f15 * f19);
        ctx_.flipTextureHorizontally = n4 == 2;
        faces_.renderEastFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 2));
    }
    if (n7 != 3 && (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x, y, z + 1, 3))) {
        float f20 = block.getLuminance(ctx_.blockView, x, y, z + 1);
        if (block.maxZ < 1.0) {
            f20 = f17;
        }
        tessellator.color(f9 * f20, f12 * f20, f15 * f20);
        ctx_.flipTextureHorizontally = n4 == 3;
        faces_.renderWestFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 3));
    }
    if (n7 != 4 && (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x - 1, y, z, 4))) {
        float f21 = block.getLuminance(ctx_.blockView, x - 1, y, z);
        if (block.minX > 0.0) {
            f21 = f17;
        }
        tessellator.color(f10 * f21, f13 * f21, f16 * f21);
        ctx_.flipTextureHorizontally = n4 == 4;
        faces_.renderNorthFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 4));
    }
    if (n7 != 5 && (ctx_.skipFaceCulling || block.isSideVisible(ctx_.blockView, x + 1, y, z, 5))) {
        float f22 = block.getLuminance(ctx_.blockView, x + 1, y, z);
        if (block.maxX < 1.0) {
            f22 = f17;
        }
        tessellator.color(f10 * f22, f13 * f22, f16 * f22);
        ctx_.flipTextureHorizontally = n4 == 5;
        faces_.renderSouthFace(block, x, y, z, block.getTextureId(ctx_.blockView, x, y, z, 5));
    }
    ctx_.flipTextureHorizontally = false;
    return true;
}

} // namespace net::minecraft::client::render::block
