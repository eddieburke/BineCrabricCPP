#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/FireBlock.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {
bool FireBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z) {
    Tessellator& tessellator = *ctx_.tess;
    const int texture = ctx_.resolveTexture(0, block.getTexture(0));
    const net::minecraft::block::TerrainAtlasUv uv = net::minecraft::block::Block::terrainTileUv(texture);
    const int texU = net::minecraft::block::Block::textureAtlasU(texture);
    const int texV = net::minecraft::block::Block::textureAtlasV(texture);
    float brightness = block.getLuminance(ctx_.blockView, x, y, z);
    tessellator.color(brightness, brightness, brightness);
    double uMin = uv.uMin;
    double uMax = uv.uMax;
    double vMin = uv.vMin;
    double vMax = uv.vMax;
    float flameHeight = 1.4f;
    if (ctx_.blockView->shouldSuffocate(x, y - 1, z) ||
        net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x, y - 1, z)) {
        double flameXHigh = (double) x + 0.5 + 0.2;
        double flameXLow = (double) x + 0.5 - 0.2;
        double flameZHigh = (double) z + 0.5 + 0.2;
        double flameZLow = (double) z + 0.5 - 0.2;
        double quadXMin = (double) x + 0.5 - 0.3;
        double quadXMax = (double) x + 0.5 + 0.3;
        double quadZLow = (double) z + 0.5 - 0.3;
        double quadZHigh = (double) z + 0.5 + 0.3;
        tessellator.vertex(quadXMin, (float) y + flameHeight, z + 1, uMax, vMin);
        tessellator.vertex(flameXHigh, y + 0, z + 1, uMax, vMax);
        tessellator.vertex(flameXHigh, y + 0, z + 0, uMin, vMax);
        tessellator.vertex(quadXMin, (float) y + flameHeight, z + 0, uMin, vMin);
        tessellator.vertex(quadXMax, (float) y + flameHeight, z + 0, uMax, vMin);
        tessellator.vertex(flameXLow, y + 0, z + 0, uMax, vMax);
        tessellator.vertex(flameXLow, y + 0, z + 1, uMin, vMax);
        tessellator.vertex(quadXMax, (float) y + flameHeight, z + 1, uMin, vMin);
        uMin = (float) texU / 256.0f;
        uMax = ((float) texU + 15.99f) / 256.0f;
        vMin = (float) (texV + 16) / 256.0f;
        vMax = ((float) texV + 15.99f + 16.0f) / 256.0f;
        tessellator.vertex(x + 1, (float) y + flameHeight, quadZHigh, uMax, vMin);
        tessellator.vertex(x + 1, y + 0, flameZLow, uMax, vMax);
        tessellator.vertex(x + 0, y + 0, flameZLow, uMin, vMax);
        tessellator.vertex(x + 0, (float) y + flameHeight, quadZHigh, uMin, vMin);
        tessellator.vertex(x + 0, (float) y + flameHeight, quadZLow, uMax, vMin);
        tessellator.vertex(x + 0, y + 0, flameZHigh, uMax, vMax);
        tessellator.vertex(x + 1, y + 0, flameZHigh, uMin, vMax);
        tessellator.vertex(x + 1, (float) y + flameHeight, quadZLow, uMin, vMin);
        flameXHigh = (double) x + 0.5 - 0.5;
        flameXLow = (double) x + 0.5 + 0.5;
        flameZHigh = (double) z + 0.5 - 0.5;
        flameZLow = (double) z + 0.5 + 0.5;
        quadXMin = (double) x + 0.5 - 0.4;
        quadXMax = (double) x + 0.5 + 0.4;
        quadZLow = (double) z + 0.5 - 0.4;
        quadZHigh = (double) z + 0.5 + 0.4;
        tessellator.vertex(quadXMin, (float) y + flameHeight, z + 0, uMin, vMin);
        tessellator.vertex(flameXHigh, y + 0, z + 0, uMin, vMax);
        tessellator.vertex(flameXHigh, y + 0, z + 1, uMax, vMax);
        tessellator.vertex(quadXMin, (float) y + flameHeight, z + 1, uMax, vMin);
        tessellator.vertex(quadXMax, (float) y + flameHeight, z + 1, uMin, vMin);
        tessellator.vertex(flameXLow, y + 0, z + 1, uMin, vMax);
        tessellator.vertex(flameXLow, y + 0, z + 0, uMax, vMax);
        tessellator.vertex(quadXMax, (float) y + flameHeight, z + 0, uMax, vMin);
        uMin = (float) texU / 256.0f;
        uMax = ((float) texU + 15.99f) / 256.0f;
        vMin = (float) texV / 256.0f;
        vMax = ((float) texV + 15.99f) / 256.0f;
        tessellator.vertex(x + 0, (float) y + flameHeight, quadZHigh, uMin, vMin);
        tessellator.vertex(x + 0, y + 0, flameZLow, uMin, vMax);
        tessellator.vertex(x + 1, y + 0, flameZLow, uMax, vMax);
        tessellator.vertex(x + 1, (float) y + flameHeight, quadZHigh, uMax, vMin);
        tessellator.vertex(x + 1, (float) y + flameHeight, quadZLow, uMin, vMin);
        tessellator.vertex(x + 1, y + 0, flameZHigh, uMin, vMax);
        tessellator.vertex(x + 0, y + 0, flameZHigh, uMax, vMax);
        tessellator.vertex(x + 0, (float) y + flameHeight, quadZLow, uMax, vMin);
    } else {
        double swappedU;
        float flameWidth = 0.2f;
        float groundInset = 0.0625f;
        if (((x + y + z) & 1) == 1) {
            uMin = (float) texU / 256.0f;
            uMax = ((float) texU + 15.99f) / 256.0f;
            vMin = (float) (texV + 16) / 256.0f;
            vMax = ((float) texV + 15.99f + 16.0f) / 256.0f;
        }
        if ((((x / 2) + (y / 2) + (z / 2)) & 1) == 1) {
            swappedU = uMax;
            uMax = uMin;
            uMin = swappedU;
        }
        if (net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x - 1, y, z)) {
            tessellator.vertex((float) x + flameWidth, (float) y + flameHeight + groundInset, z + 1, uMax, vMin);
            tessellator.vertex(x + 0, (float) (y + 0) + groundInset, z + 1, uMax, vMax);
            tessellator.vertex(x + 0, (float) (y + 0) + groundInset, z + 0, uMin, vMax);
            tessellator.vertex((float) x + flameWidth, (float) y + flameHeight + groundInset, z + 0, uMin, vMin);
            tessellator.vertex((float) x + flameWidth, (float) y + flameHeight + groundInset, z + 0, uMin, vMin);
            tessellator.vertex(x + 0, (float) (y + 0) + groundInset, z + 0, uMin, vMax);
            tessellator.vertex(x + 0, (float) (y + 0) + groundInset, z + 1, uMax, vMax);
            tessellator.vertex((float) x + flameWidth, (float) y + flameHeight + groundInset, z + 1, uMax, vMin);
        }
        if (net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x + 1, y, z)) {
            tessellator.vertex((float) (x + 1) - flameWidth, (float) y + flameHeight + groundInset, z + 0, uMin, vMin);
            tessellator.vertex(x + 1 - 0, (float) (y + 0) + groundInset, z + 0, uMin, vMax);
            tessellator.vertex(x + 1 - 0, (float) (y + 0) + groundInset, z + 1, uMax, vMax);
            tessellator.vertex((float) (x + 1) - flameWidth, (float) y + flameHeight + groundInset, z + 1, uMax, vMin);
            tessellator.vertex((float) (x + 1) - flameWidth, (float) y + flameHeight + groundInset, z + 1, uMax, vMin);
            tessellator.vertex(x + 1 - 0, (float) (y + 0) + groundInset, z + 1, uMax, vMax);
            tessellator.vertex(x + 1 - 0, (float) (y + 0) + groundInset, z + 0, uMin, vMax);
            tessellator.vertex((float) (x + 1) - flameWidth, (float) y + flameHeight + groundInset, z + 0, uMin, vMin);
        }
        if (net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x, y, z - 1)) {
            tessellator.vertex(x + 0, (float) y + flameHeight + groundInset, (float) z + flameWidth, uMax, vMin);
            tessellator.vertex(x + 0, (float) (y + 0) + groundInset, z + 0, uMax, vMax);
            tessellator.vertex(x + 1, (float) (y + 0) + groundInset, z + 0, uMin, vMax);
            tessellator.vertex(x + 1, (float) y + flameHeight + groundInset, (float) z + flameWidth, uMin, vMin);
            tessellator.vertex(x + 1, (float) y + flameHeight + groundInset, (float) z + flameWidth, uMin, vMin);
            tessellator.vertex(x + 1, (float) (y + 0) + groundInset, z + 0, uMin, vMax);
            tessellator.vertex(x + 0, (float) (y + 0) + groundInset, z + 0, uMax, vMax);
            tessellator.vertex(x + 0, (float) y + flameHeight + groundInset, (float) z + flameWidth, uMax, vMin);
        }
        if (net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x, y, z + 1)) {
            tessellator.vertex(x + 1, (float) y + flameHeight + groundInset, (float) (z + 1) - flameWidth, uMin, vMin);
            tessellator.vertex(x + 1, (float) (y + 0) + groundInset, z + 1 - 0, uMin, vMax);
            tessellator.vertex(x + 0, (float) (y + 0) + groundInset, z + 1 - 0, uMax, vMax);
            tessellator.vertex(x + 0, (float) y + flameHeight + groundInset, (float) (z + 1) - flameWidth, uMax, vMin);
            tessellator.vertex(x + 0, (float) y + flameHeight + groundInset, (float) (z + 1) - flameWidth, uMax, vMin);
            tessellator.vertex(x + 0, (float) (y + 0) + groundInset, z + 1 - 0, uMax, vMax);
            tessellator.vertex(x + 1, (float) (y + 0) + groundInset, z + 1 - 0, uMin, vMax);
            tessellator.vertex(x + 1, (float) y + flameHeight + groundInset, (float) (z + 1) - flameWidth, uMin, vMin);
        }
        if (net::minecraft::block::FireBlock::isFlammable(ctx_.blockView, x, y + 1, z)) {
            const double blockXMax = (double) x + 0.5 + 0.5;
            const double blockXMin = (double) x + 0.5 - 0.5;
            const double blockZMax = (double) z + 0.5 + 0.5;
            const double blockZMin = (double) z + 0.5 - 0.5;
            const double westX = (double) x + 0.5 - 0.5;
            const double eastX = (double) x + 0.5 + 0.5;
            const double northZ = (double) z + 0.5 - 0.5;
            const double southZ = (double) z + 0.5 + 0.5;
            uMin = (float) texU / 256.0f;
            uMax = ((float) texU + 15.99f) / 256.0f;
            vMin = (float) texV / 256.0f;
            vMax = ((float) texV + 15.99f) / 256.0f;
            flameHeight = -0.2f;
            if (((x + ++y + z) & 1) == 0) {
                tessellator.vertex(westX, (float) y + flameHeight, z + 0, uMax, vMin);
                tessellator.vertex(blockXMax, y + 0, z + 0, uMax, vMax);
                tessellator.vertex(blockXMax, y + 0, z + 1, uMin, vMax);
                tessellator.vertex(westX, (float) y + flameHeight, z + 1, uMin, vMin);
                uMin = (float) texU / 256.0f;
                uMax = ((float) texU + 15.99f) / 256.0f;
                vMin = (float) (texV + 16) / 256.0f;
                vMax = ((float) texV + 15.99f + 16.0f) / 256.0f;
                tessellator.vertex(eastX, (float) y + flameHeight, z + 1, uMax, vMin);
                tessellator.vertex(blockXMin, y + 0, z + 1, uMax, vMax);
                tessellator.vertex(blockXMin, y + 0, z + 0, uMin, vMax);
                tessellator.vertex(eastX, (float) y + flameHeight, z + 0, uMin, vMin);
            } else {
                tessellator.vertex(x + 0, (float) y + flameHeight, southZ, uMax, vMin);
                tessellator.vertex(x + 0, y + 0, blockZMin, uMax, vMax);
                tessellator.vertex(x + 1, y + 0, blockZMin, uMin, vMax);
                tessellator.vertex(x + 1, (float) y + flameHeight, southZ, uMin, vMin);
                uMin = (float) texU / 256.0f;
                uMax = ((float) texU + 15.99f) / 256.0f;
                vMin = (float) (texV + 16) / 256.0f;
                vMax = ((float) texV + 15.99f + 16.0f) / 256.0f;
                tessellator.vertex(x + 1, (float) y + flameHeight, northZ, uMax, vMin);
                tessellator.vertex(x + 1, y + 0, blockZMax, uMax, vMax);
                tessellator.vertex(x + 0, y + 0, blockZMax, uMin, vMax);
                tessellator.vertex(x + 0, (float) y + flameHeight, northZ, uMin, vMin);
            }
        }
    }
    return true;
}
}  // namespace net::minecraft::client::render::block
